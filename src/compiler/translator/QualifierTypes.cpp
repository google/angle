//
// Copyright (c) 2002-2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "QualifierTypes.h"

#include "Diagnostics.h"

namespace
{

bool IsScopeQualifier(TQualifier qualifier)
{
    return qualifier == EvqGlobal || qualifier == EvqTemporary;
}

bool IsScopeQualifierWrapper(const TQualifierWrapperBase *qualifier)
{
    if (qualifier->getType() != QtStorage)
        return false;
    const TStorageQualifierWrapper *storageQualifier =
        static_cast<const TStorageQualifierWrapper *>(qualifier);
    TQualifier q = storageQualifier->getQualifier();
    return IsScopeQualifier(q);
}

// Returns true if the invariant for the qualifier sequence holds
bool IsInvariantCorrect(const std::vector<const TQualifierWrapperBase *> &qualifiers)
{
    // We should have at least one qualifier.
    // The first qualifier always tells the scope.
    return qualifiers.size() >= 1 && IsScopeQualifierWrapper(qualifiers[0]);
}

// Returns true if there are qualifiers which have been specified multiple times
bool HasRepeatingQualifiers(const std::vector<const TQualifierWrapperBase *> &qualifiers,
                            std::string *errorMessage)
{
    bool invariantFound     = false;
    bool precisionFound     = false;
    bool layoutFound        = false;
    bool interpolationFound = false;

    // The iteration starts from one since the first qualifier only reveals the scope of the
    // expression. It is inserted first whenever the sequence gets created.
    for (size_t i = 1; i < qualifiers.size(); ++i)
    {
        switch (qualifiers[i]->getType())
        {
            case QtInvariant:
            {
                if (invariantFound)
                {
                    *errorMessage = "The invariant qualifier specified multiple times.";
                    return true;
                }
                invariantFound = true;
                break;
            }
            case QtPrecision:
            {
                if (precisionFound)
                {
                    *errorMessage = "The precision qualifier specified multiple times.";
                    return true;
                }
                precisionFound = true;
                break;
            }
            case QtLayout:
            {
                if (layoutFound)
                {
                    *errorMessage = "The layout qualifier specified multiple times.";
                    return true;
                }
                layoutFound = true;
                break;
            }
            case QtInterpolation:
            {
                // 'centroid' is treated as a storage qualifier
                // 'flat centroid' will be squashed to 'flat'
                // 'smooth centroid' will be squashed to 'centroid'
                if (interpolationFound)
                {
                    *errorMessage = "The interpolation qualifier specified multiple times.";
                    return true;
                }
                interpolationFound = true;
                break;
            }
            case QtStorage:
            {
                // Go over all of the storage qualifiers up until the current one and check for
                // repetitions.
                TQualifier currentQualifier =
                    static_cast<const TStorageQualifierWrapper *>(qualifiers[i])->getQualifier();
                for (size_t j = 1; j < i; ++j)
                {
                    if (qualifiers[j]->getType() == QtStorage)
                    {
                        const TStorageQualifierWrapper *previousQualifierWrapper =
                            static_cast<const TStorageQualifierWrapper *>(qualifiers[j]);
                        TQualifier previousQualifier = previousQualifierWrapper->getQualifier();
                        if (currentQualifier == previousQualifier)
                        {
                            *errorMessage = previousQualifierWrapper->getQualifierString().c_str();
                            *errorMessage += " specified multiple times";
                            return true;
                        }
                    }
                }
                break;
            }
            default:
                UNREACHABLE();
        }
    }

    return false;
}

// GLSL ES 3.00_6, 4.7 Order of Qualification
// The correct order of qualifiers is:
// invariant-qualifier interpolation-qualifier storage-qualifier precision-qualifier
// layout-qualifier has to be before storage-qualifier.
bool AreQualifiersInOrder(const std::vector<const TQualifierWrapperBase *> &qualifiers,
                          std::string *errorMessage)
{
    bool foundInterpolation = false;
    bool foundStorage       = false;
    bool foundPrecision     = false;
    for (size_t i = 1; i < qualifiers.size(); ++i)
    {
        switch (qualifiers[i]->getType())
        {
            case QtInvariant:
                if (foundInterpolation || foundStorage || foundPrecision)
                {
                    *errorMessage = "The invariant qualifier has to be first in the expression.";
                    return false;
                }
                break;
            case QtInterpolation:
                if (foundStorage)
                {
                    *errorMessage = "Storage qualifiers have to be after interpolation qualifiers.";
                    return false;
                }
                else if (foundPrecision)
                {
                    *errorMessage =
                        "Precision qualifiers have to be after interpolation qualifiers.";
                    return false;
                }
                foundInterpolation = true;
                break;
            case QtLayout:
                if (foundStorage)
                {
                    *errorMessage = "Storage qualifiers have to be after layout qualifiers.";
                    return false;
                }
                else if (foundPrecision)
                {
                    *errorMessage = "Precision qualifiers have to be after layout qualifiers.";
                    return false;
                }
                break;
            case QtStorage:
                if (foundPrecision)
                {
                    *errorMessage = "Precision qualifiers have to be after storage qualifiers.";
                    return false;
                }
                foundStorage = true;
                break;
            case QtPrecision:
                foundPrecision = true;
                break;
            default:
                UNREACHABLE();
        }
    }
    return true;
}

}  // namespace

TTypeQualifier::TTypeQualifier(TQualifier scope, const TSourceLoc &loc)
    : layoutQualifier(TLayoutQualifier::create()),
      precision(EbpUndefined),
      qualifier(scope),
      invariant(false),
      line(loc)
{
    ASSERT(IsScopeQualifier(qualifier));
}

TTypeQualifierBuilder::TTypeQualifierBuilder(const TStorageQualifierWrapper *scope)
{
    ASSERT(IsScopeQualifier(scope->getQualifier()));
    mQualifiers.push_back(scope);
}

void TTypeQualifierBuilder::appendQualifier(const TQualifierWrapperBase *qualifier)
{
    mQualifiers.push_back(qualifier);
}

bool TTypeQualifierBuilder::checkOrderIsValid(TDiagnostics *diagnostics) const
{
    std::string errorMessage;
    if (HasRepeatingQualifiers(mQualifiers, &errorMessage))
    {
        diagnostics->error(mQualifiers[0]->getLine(), "qualifier sequence", errorMessage.c_str(),
                           "");
        return false;
    }

    if (!AreQualifiersInOrder(mQualifiers, &errorMessage))
    {
        diagnostics->error(mQualifiers[0]->getLine(), "qualifier sequence", errorMessage.c_str(),
                           "");
        return false;
    }

    return true;
}

TTypeQualifier TTypeQualifierBuilder::getParameterTypeQualifier(TDiagnostics *diagnostics) const
{
    ASSERT(IsInvariantCorrect(mQualifiers));
    ASSERT(static_cast<const TStorageQualifierWrapper *>(mQualifiers[0])->getQualifier() ==
           EvqTemporary);

    TTypeQualifier typeQualifier(EvqTemporary, mQualifiers[0]->getLine());

    if (!checkOrderIsValid(diagnostics))
    {
        return typeQualifier;
    }

    for (size_t i = 1; i < mQualifiers.size(); ++i)
    {
        const TQualifierWrapperBase *qualifier = mQualifiers[i];
        bool isQualifierValid                  = false;
        switch (qualifier->getType())
        {
            case QtInvariant:
            case QtInterpolation:
            case QtLayout:
                break;
            case QtStorage:
                isQualifierValid = joinParameterStorageQualifier(
                    &typeQualifier.qualifier,
                    static_cast<const TStorageQualifierWrapper *>(qualifier)->getQualifier());
                break;
            case QtPrecision:
                isQualifierValid = true;
                typeQualifier.precision =
                    static_cast<const TPrecisionQualifierWrapper *>(qualifier)->getQualifier();
                ASSERT(typeQualifier.precision != EbpUndefined);
                break;
            default:
                UNREACHABLE();
        }
        if (!isQualifierValid)
        {
            const TString &qualifierString = qualifier->getQualifierString();
            diagnostics->error(qualifier->getLine(), "invalid parameter qualifier",
                               qualifierString.c_str(), "");
            break;
        }
    }

    switch (typeQualifier.qualifier)
    {
        case EvqIn:
        case EvqConstReadOnly:  // const in
        case EvqOut:
        case EvqInOut:
            break;
        case EvqConst:
            typeQualifier.qualifier = EvqConstReadOnly;
            break;
        case EvqTemporary:
            // no qualifier has been specified, set it to EvqIn which is the default
            typeQualifier.qualifier = EvqIn;
            break;
        default:
            diagnostics->error(mQualifiers[0]->getLine(), "Invalid parameter qualifier ",
                               getQualifierString(typeQualifier.qualifier), "");
    }
    return typeQualifier;
}

TTypeQualifier TTypeQualifierBuilder::getVariableTypeQualifier(TDiagnostics *diagnostics) const
{
    ASSERT(IsInvariantCorrect(mQualifiers));

    TQualifier scope =
        static_cast<const TStorageQualifierWrapper *>(mQualifiers[0])->getQualifier();
    TTypeQualifier typeQualifier = TTypeQualifier(scope, mQualifiers[0]->getLine());

    if (!checkOrderIsValid(diagnostics))
    {
        return typeQualifier;
    }

    for (size_t i = 1; i < mQualifiers.size(); ++i)
    {
        const TQualifierWrapperBase *qualifier = mQualifiers[i];
        bool isQualifierValid                  = false;
        switch (qualifier->getType())
        {
            case QtInvariant:
                isQualifierValid        = true;
                typeQualifier.invariant = true;
                break;
            case QtInterpolation:
            {
                switch (typeQualifier.qualifier)
                {
                    case EvqGlobal:
                        isQualifierValid = true;
                        typeQualifier.qualifier =
                            static_cast<const TInterpolationQualifierWrapper *>(qualifier)
                                ->getQualifier();
                        break;
                    default:
                        isQualifierValid = false;
                }
                break;
            }
            case QtLayout:
                isQualifierValid = true;
                typeQualifier.layoutQualifier =
                    static_cast<const TLayoutQualifierWrapper *>(qualifier)->getQualifier();
                break;
            case QtStorage:
                isQualifierValid = joinVariableStorageQualifier(
                    &typeQualifier.qualifier,
                    static_cast<const TStorageQualifierWrapper *>(qualifier)->getQualifier());
                break;
            case QtPrecision:
                isQualifierValid = true;
                typeQualifier.precision =
                    static_cast<const TPrecisionQualifierWrapper *>(qualifier)->getQualifier();
                ASSERT(typeQualifier.precision != EbpUndefined);
                break;
            default:
                UNREACHABLE();
        }
        if (!isQualifierValid)
        {
            const TString &qualifierString = qualifier->getQualifierString();
            diagnostics->error(qualifier->getLine(), "invalid qualifier combination",
                               qualifierString.c_str(), "");
            break;
        }
    }
    return typeQualifier;
}

bool TTypeQualifierBuilder::joinVariableStorageQualifier(TQualifier *joinedQualifier,
                                                         TQualifier storageQualifier) const
{
    switch (*joinedQualifier)
    {
        case EvqGlobal:
            *joinedQualifier = storageQualifier;
            break;
        case EvqTemporary:
        {
            switch (storageQualifier)
            {
                case EvqConst:
                    *joinedQualifier = storageQualifier;
                    break;
                default:
                    return false;
            }
            break;
        }
        case EvqSmooth:
        {
            switch (storageQualifier)
            {
                case EvqCentroid:
                    *joinedQualifier = EvqCentroid;
                    break;
                case EvqVertexOut:
                    *joinedQualifier = EvqSmoothOut;
                    break;
                case EvqFragmentIn:
                    *joinedQualifier = EvqSmoothIn;
                    break;
                default:
                    return false;
            }
            break;
        }
        case EvqFlat:
        {
            switch (storageQualifier)
            {
                case EvqCentroid:
                    *joinedQualifier = EvqFlat;
                    break;
                case EvqVertexOut:
                    *joinedQualifier = EvqFlatOut;
                    break;
                case EvqFragmentIn:
                    *joinedQualifier = EvqFlatIn;
                    break;
                default:
                    return false;
            }
            break;
        }
        case EvqCentroid:
        {
            switch (storageQualifier)
            {
                case EvqVertexOut:
                    *joinedQualifier = EvqCentroidOut;
                    break;
                case EvqFragmentIn:
                    *joinedQualifier = EvqCentroidIn;
                    break;
                default:
                    return false;
            }
            break;
        }
        default:
            return false;
    }
    return true;
}

bool TTypeQualifierBuilder::joinParameterStorageQualifier(TQualifier *joinedQualifier,
                                                          TQualifier storageQualifier) const
{
    switch (*joinedQualifier)
    {
        case EvqTemporary:
            *joinedQualifier = storageQualifier;
            break;
        case EvqConst:
        {
            switch (storageQualifier)
            {
                case EvqIn:
                    *joinedQualifier = EvqConstReadOnly;
                    break;
                default:
                    return false;
            }
            break;
        }
        default:
            return false;
    }
    return true;
}
