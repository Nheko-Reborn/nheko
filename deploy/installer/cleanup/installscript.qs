function Component()
{
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    try
    {
        if( installer.value("os") === "win" )
        {
            /**
             * Cleanup AppData and registry
             */
            component.addElevatedOperation("Execute","UNDOEXECUTE","cmd /C reg delete HKEY_CURRENT_USER\Software\nheko\nheko /f");
            var localappdata = installer.environmentVariable("LOCALAPPDATA");
            if( localappdata != "" )
            {
                component.addElevatedOperation("Execute","UNDOEXECUTE","cmd /C rmdir "+localappdata+"\nheko /f");
            }
        }
    }
    catch( e )
    {
        print( e );
    }
}
