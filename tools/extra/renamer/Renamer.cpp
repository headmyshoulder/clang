/*
 * main.cpp
 * Date: 2012-10-24
 * Author: Karsten Ahnert (karsten.ahnert@gmx.de)
 */
//=- examples/rename-method/RenameMethod.cpp ------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// A small example tool that uses AST matchers to find calls to the Get() method
// in subclasses of ElementsBase and replaces them with calls to Front().
//
//===----------------------------------------------------------------------===//

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/FileManager.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/JSONCompilationDatabase.h>
#include <clang/Tooling/Refactoring.h>

#include <iostream>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace llvm;


using namespace std;

// Implements a callback that replaces the calls for the AST
// nodes we matched.
class CallRenamer : public MatchFinder::MatchCallback
{
public:

    CallRenamer(Replacements *Replace)
        : Replace(Replace) {}

    // This method is called every time the registered matcher matches
    // on the AST.
    virtual void run(const MatchFinder::MatchResult &result)
    {
        cout << "CallRenamer" << endl;

        SourceManager &sm = ( *result.SourceManager );

        const MemberExpr *mExpr = result.Nodes.getStmtAs<MemberExpr>("member");
        SourceLocation mlog = mExpr->getMemberLoc();

        clang::FullSourceLoc fsl( mlog , sm );
        std::string filename = "AAA";
        if( fsl.isValid() )
        {
            const clang::FileEntry *fe = sm.getFileEntryForID( fsl.getFileID() );
            if( !fe )
            {
                SourceLocation sl = sm.getSpellingLoc(mlog);
                if( sl.isValid() )
                {
                    clang::FullSourceLoc fsl2(sl, sm);
                    const clang::FileEntry *fe2 = sm.getFileEntryForID( fsl2.getFileID() );
                    if( fe2 )
                        filename = std::string( "XXX: " ) + fe2->getName();
                    else
                        filename = "DDD";
                }
                else
                {
                    filename = "CCC";
                }
            }
            else
            {
                filename = std::string( "YYY" ) + fe->getName();
            }
        }
        else
        {
            SourceLocation sl = sm.getSpellingLoc(mlog);
            if( sl.isValid() )
            {
                clang::FullSourceLoc fsl2(sl, sm);
                const clang::FileEntry *fe2 = sm.getFileEntryForID( fsl2.getFileID() );
                filename = std::string( "ZZZ" ) + fe2->getName();
            }
            else
            {
                filename = "BBB";
            }
        }


        
        // FileID fid = sm.getFileID( M->getMemberLoc() );
        




        DeclarationNameInfo dnInfo = mExpr->getMemberNameInfo();
        cout << "Variablenname : " << dnInfo.getName().getAsString() << " ";
        cout << "Filename : " << filename;
        cout << "\n";
        


        
        // for( SourceManager::fileinfo_iterator iter = sm.fileinfo_begin() ; iter != sm.fileinfo_end() ; ++iter )
        // {
        //     const FileEntry &entry = *( iter->first );
        //     cout << entry.getName() << " " << entry.getDir() << "\n";
        // }
        // cout << "endfiles" << endl;
        

        


        // USE *Result.SourceManager.fileinfo_begin()

        // Replace->insert(
        //     // Replacements are a source manager independent way to express
        //     // transformation on the source.
        //     Replacement(*Result.SourceManager,
        //                 // Replace the range of the member name...
        //                 CharSourceRange::getTokenRange(
        //                     SourceRange(M->getMemberLoc())),
        //                 // ... with "Front".
        //                 "Front"));
    }

private:

    // Replacements are the RefactoringTool's way to keep track of code
    // transformations, deduplicate them and apply them to the code when
    // the tool has finished with all translation units.
    Replacements *Replace;
};

// Implements a callback that replaces the decls for the AST
// nodes we matched.
class DeclRenamer : public MatchFinder::MatchCallback
{
public:

    DeclRenamer(Replacements *Replace) : Replace(Replace) {}

    // This method is called every time the registered matcher matches
    // on the AST.
    virtual void run(const MatchFinder::MatchResult &Result)
    {
        const CXXMethodDecl *D = Result.Nodes.getDeclAs<CXXMethodDecl>("method");
        // We can assume D is non-null, because the ast matchers guarantee
        // that a node with this type was bound, as the matcher would otherwise
        // not match.

        cout << "DeclRenamer" << endl;

        // Replace->insert(
        //     // Replacements are a source manager independent way to express
        //     // transformation on the source.
        //     Replacement(*Result.SourceManager,
        //                 // Replace the range of the declarator identifier...
        //                 CharSourceRange::getTokenRange(
        //                     SourceRange(D->getLocation())),
        //                 // ... with "Front".
        //                 "Front"));
    }

private:
    // Replacements are the RefactoringTool's way to keep track of code
    // transformations, deduplicate them and apply them to the code when
    // the tool has finished with all translation units.
    Replacements *Replace;

};

int main(int argc, const char **argv) {

    std::string ErrorMessage;
    OwningPtr< CompilationDatabase > Compilations( JSONCompilationDatabase::loadFromFile( "compile_commands.json" , ErrorMessage ) );
    if( !Compilations )
        llvm::report_fatal_error( ErrorMessage );


    RefactoringTool Tool( *Compilations , Compilations->getAllFiles() );
    ast_matchers::MatchFinder Finder;
    CallRenamer CallCallback(&Tool.getReplacements());
    Finder.addMatcher(
        memberExpr(
            // Where the callee is a method called "Get"...
            anything()  ,
            id( "member" , memberExpr() ) ) ,
        &CallCallback );

    DeclRenamer DeclCallback( &Tool.getReplacements() );
    Finder.addMatcher(
        // Match declarations...
        id("method", methodDecl(hasName("_uebergangsfunktion"),
                                ofClass( isDerivedFrom( "ITransition" ) ) ) ) ,
        &DeclCallback);

    return Tool.run(newFrontendActionFactory(&Finder));
}



/*

 OK * Git Repository, with git submodules
 OK * Einbauen in clang Tree
 OK * Nur Lesen auf compilation_database.json
 OK * Nimm all Dateien auf compilation_database.json
 NO * Anderes Replacement
   OK * wuerde (triviales) Umschreiben RefactoringTool bedeuten
   OK * lieber lokal den Tree kopieren
 * Ausgeben von Informationen in Callbacks
   * Welche Datei
   * Welche TranslationUnit
 * Suchen nach MemberDecl
 * Suchen nach Member

 */
