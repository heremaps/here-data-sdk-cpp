# This awk script is used to filter the output of the xcodebuild command.
BEGIN { mode = "START"; IGNORECASE=1; }
{
    shouldPrint=0

    if($1 == "CompileC") print "Compiling",$3,$5;
    else if($1 == "===" || $1 == "**" || $1 == "GenerateDSYMFile" || $1 == "CpResource" || $1 == "CompileStoryboard" || $1 == "CopyStringsFile" || $1 == "CompileAssetCatalog" || $1 == "ProcessInfoPlistFile" || $1 == "ProcessProductPackaging"|| $1 == "Touch" || $1 == "CodeSign" || $1 == "Validate" || $1 == "ProcessPCH" || $1 == "CreateUniversalBinary" || $1 == "Libtool" )
    {
        shouldPrint=1;
    }

    if( /CLEAN TARGET/ ) { mode = "CLEANING"; }
    #else if( /BUILD TARGET/ ) { mode = "BUILDING"; }
    #else if( /ANALYZE TARGET/ ) { mode = "ANALYZING" }
    else if($1 == "Ld") { mode = "LINKING"; shouldPrint=1; }
    else if($1 == "Undefined") { mode = "UNDEFINED_SYMBOLS"; shouldPrint=1; }
    else if( /The following build commands failed/ ) { mode = "BUILD_FAILED"; }

    if(mode == "START")
    {
        shouldPrint=1;
    }
    else if(mode == "UNDEFINED_SYMBOLS")
    {
        if( /referenced from/ )
        {
            match($0,"\".*\"");
            print substr($0,RSTART,RLENGTH);
        }
    }
    else if(mode == "BUILD_FAILED")
    {
        shouldPrint=1;
    }

    if( shouldPrint == 0 )
    {
        if( /error:/ || / Error/ || /ERROR:/ || /warning:/ || /fail/ ) shouldPrint=1;
    }

    # ignore known errors coming from Xcode7
    #ERROR: 49: Couldn't connect to system sound server
    #Error creating notification handler for simulator graphics quality override: 1000000
    if( shouldPrint == 1 )
    {
        if( /49: Couldn't connect to system sound server/ || /creating notification handler for simulator graphics quality override: 1000000/ ) shouldPrint=0 ;
    }

    if( shouldPrint == 1 ) print;

    fflush();
}
