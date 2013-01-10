/*
 * main.cpp
 * Date: 2012-10-24
 * Author: Karsten Ahnert (karsten.ahnert@gmx.de)
 */


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


std::string getFilenameFromLocation( SourceLocation mloc , SourceManager &sm )
{
    clang::FullSourceLoc fsl( mloc , sm );
    std::string filename = "AAA";
    if( fsl.isValid() )
    {
        const clang::FileEntry *fe = sm.getFileEntryForID( fsl.getFileID() );
        if( !fe )
        {
            SourceLocation sl = sm.getSpellingLoc(mloc);
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
        SourceLocation sl = sm.getSpellingLoc(mloc);
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
    return filename;
}


class RenamerContext
{
public:

    RenamerContext( const std::string &basePath )
        :m_basePath( basePath )
    { }

    bool isMemberVariableExpr( const MemberExpr *mExpr ) const
    {
        const ValueDecl* valDecl = mExpr->getMemberDecl();
        const FieldDecl* fieldDecl = dynamic_cast< const FieldDecl* >( valDecl );
        return ( fieldDecl != 0 );
    }

    bool isValidExprSource( const MemberExpr *mExpr , SourceManager &sm ) const
    {
        std::string exprFilename = sm.getFilename( mExpr->getMemberLoc() ).str();
        std::string declFilename = sm.getFilename( mExpr->getMemberDecl()->getLocation() ).str();
        // cout << exprFilename << "\n" << declFilename << "\n" << m_basePath << " " << "\n";
        // cout << exprFilename.find( m_basePath ) << "\n";
        if( ( exprFilename.find( m_basePath ) == 1 ) && ( declFilename.find( m_basePath ) == 1 ) ) return true;
        else return false;
    }

    bool isNotPublic( const MemberExpr *mExpr ) const
    {
        const ValueDecl* valDecl = mExpr->getMemberDecl();
        AccessSpecifier ac = valDecl->getAccess();
        if( ( ac ==  AS_private ) || ( ac == AS_protected ) ) return true;
        else return false;
    }

    bool isCorrectFormatted( const std::string &name )
    {
        if( name.size() < 2 ) return false;
        if( name[ name.size() - 1 ] != '_' ) return false;
        if( !std::islower( name[0] ) ) return false;
        return true;
    }

    std::string getCorrectFormatedVariable( std::string name )
    {
        if( !std::islower( name[0] ) ) name[0] = std::tolower( name[0] );
        if( name[ name.size() - 1] != '_' ) name += '_';
        return name;
    }

private:

    std::string m_basePath;
};


class CallRenamer : public MatchFinder::MatchCallback
{
public:

    CallRenamer( Replacements *Replace , RenamerContext &context ) : Replace(Replace) , m_context( context ) {}

    virtual void run(const MatchFinder::MatchResult &result)
    {
        SourceManager &sm = ( *result.SourceManager );
        const MemberExpr *mExpr = result.Nodes.getStmtAs<MemberExpr>("member");

        DeclarationNameInfo dnInfo = mExpr->getMemberNameInfo();
        std::string name = dnInfo.getName().getAsString();

        if( m_context.isMemberVariableExpr( mExpr ) )
        {
            if( m_context.isNotPublic( mExpr ) )
            {
                if( m_context.isValidExprSource( mExpr , sm ) )
                {
                    if( !m_context.isCorrectFormatted( name ) )
                    {
                        std::string suggestion = m_context.getCorrectFormatedVariable( name )
                        cout << "\t" << sm.getFilename( mExpr->getMemberLoc() ).str() << " : "
                             << name << ", Vorschlag " << suggestion << "\n";

                        
                        // Replace->insert( Replacement( sm ,
                        //                               CharSourceRange::getTokenRange( SourceRange( mExpr->getMemberLoc() ) ) ,
                        //                               suggestion.c_str() )
                        //     );

                    }
                }
            }
        }


    }

private:

    Replacements *Replace;
    RenamerContext &m_context;
};




int main(int argc, const char **argv) {

    std::string ErrorMessage;
    OwningPtr< CompilationDatabase > Compilations( JSONCompilationDatabase::loadFromFile( "compile_commands.json" , ErrorMessage ) );
    if( !Compilations )
        llvm::report_fatal_error( ErrorMessage );


    RefactoringTool Tool( *Compilations , Compilations->getAllFiles() );
    RenamerContext renamerContext( "home/karsten/tc-supertoll/trunk/src/SuperToll" );
    ast_matchers::MatchFinder Finder;
    CallRenamer CallCallback( &Tool.getReplacements() , renamerContext );
    Finder.addMatcher(
        memberExpr(
            unless(memberCallExpr(anything()))  ,
            id( "member" , memberExpr() ) ) ,
        &CallCallback );

    // ToDo : add an Matcher for the declarations


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
