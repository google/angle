//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/Initialize.h"
#include "compiler/ParseHelper.h"
#include "compiler/ShHandle.h"

static bool InitializeSymbolTable(
        const TBuiltInStrings& builtInStrings,
        EShLanguage language, EShSpec spec, const TBuiltInResource& resources,
        TInfoSink& infoSink, TSymbolTable& symbolTable)
{
    TIntermediate intermediate(infoSink);
    TExtensionBehavior extBehavior;
    TParseContext parseContext(symbolTable, extBehavior, intermediate, language, spec, infoSink);

    GlobalParseContext = &parseContext;

    setInitialState();

    assert(symbolTable.isEmpty());       
    //
    // Parse the built-ins.  This should only happen once per
    // language symbol table.
    //
    // Push the symbol table to give it an initial scope.  This
    // push should not have a corresponding pop, so that built-ins
    // are preserved, and the test for an empty table fails.
    //
    symbolTable.push();
    
    //Initialize the Preprocessor
    if (InitPreprocessor())
    {
        infoSink.info.message(EPrefixInternalError,  "Unable to intialize the Preprocessor");
        return false;
    }

    for (TBuiltInStrings::const_iterator i = builtInStrings.begin(); i != builtInStrings.end(); ++i)
    {
        const char* builtInShaders = i->c_str();
        int builtInLengths = static_cast<int>(i->size());
        if (builtInLengths <= 0)
          continue;

        if (PaParseStrings(&builtInShaders, &builtInLengths, 1, parseContext) != 0)
        {
            infoSink.info.message(EPrefixInternalError, "Unable to parse built-ins");
            return false;
        }
    }

    IdentifyBuiltIns(language, spec, resources, symbolTable);

    FinalizePreprocessor();

    return true;
}

static void DefineExtensionMacros(const TExtensionBehavior& extBehavior)
{
    for (TExtensionBehavior::const_iterator iter = extBehavior.begin();
         iter != extBehavior.end(); ++iter) {
        PredefineIntMacro(iter->first.c_str(), 1);
    }
}

bool TCompiler::Init(const TBuiltInResource& resources)
{
    // Generate built-in symbol table.
    if (!InitBuiltInSymbolTable(resources))
        return false;

    InitExtensionBehavior(resources, extensionBehavior);
    return true;
}

bool TCompiler::compile(const char* const shaderStrings[],
                        const int numStrings,
                        int compileOptions)
{
    clearResults();

    if (numStrings == 0)
        return true;

    TIntermediate intermediate(infoSink);
    TParseContext parseContext(symbolTable, extensionBehavior, intermediate,
                               language, spec, infoSink);
    GlobalParseContext = &parseContext;
    setInitialState();

    // Initialize preprocessor.
    InitPreprocessor();
    DefineExtensionMacros(extensionBehavior);

    // We preserve symbols at the built-in level from compile-to-compile.
    // Start pushing the user-defined symbols at global level.
    symbolTable.push();
    if (!symbolTable.atGlobalLevel())
        infoSink.info.message(EPrefixInternalError, "Wrong symbol table level");

    // Parse shader.
    bool success =
        (PaParseStrings(shaderStrings, 0, numStrings, parseContext) == 0) &&
        (parseContext.treeRoot != NULL);
    if (success) {
        success = intermediate.postProcess(parseContext.treeRoot);

        if (success && (compileOptions & EShOptIntermediateTree))
            intermediate.outputTree(parseContext.treeRoot);

        if (success && (compileOptions & EShOptObjectCode))
            translate(parseContext.treeRoot);

        if (success && (compileOptions & EShOptAttribsUniforms))
            collectAttribsUniforms(parseContext.treeRoot);
    }

    // Cleanup memory.
    intermediate.remove(parseContext.treeRoot);
    // Ensure symbol table is returned to the built-in level,
    // throwing away all but the built-ins.
    while (!symbolTable.atBuiltInLevel())
        symbolTable.pop();
    FinalizePreprocessor();

    return success;
}

bool TCompiler::InitBuiltInSymbolTable(const TBuiltInResource& resources)
{
    TBuiltIns builtIns;

    builtIns.initialize(language, spec, resources);
    return InitializeSymbolTable(builtIns.getBuiltInStrings(), language, spec, resources, infoSink, symbolTable);
}

void TCompiler::clearResults()
{
    infoSink.info.erase();
    infoSink.obj.erase();
    infoSink.debug.erase();

    attribs.clear();
    uniforms.clear();
}

void TCompiler::collectAttribsUniforms(TIntermNode* root)
{
    CollectAttribsUniforms collect(attribs, uniforms);
    root->traverse(&collect);
}

