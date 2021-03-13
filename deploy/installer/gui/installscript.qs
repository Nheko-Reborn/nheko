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
             * Start Menu Shortcut
             */
            component.addOperation( "CreateShortcut", "@TargetDir@\\nheko.exe", "@StartMenuDir@\\nheko.lnk",
                                    "workingDirectory=@TargetDir@", "iconPath=@TargetDir@\\nheko.exe",
                                    "iconId=0", "description=Desktop client for the Matrix protocol");

            /**
             * Desktop Shortcut
             */
            component.addOperation( "CreateShortcut", "@TargetDir@\\nheko.exe", "@DesktopDir@\\nheko.lnk",
                                    "workingDirectory=@TargetDir@", "iconPath=@TargetDir@\\nheko.exe",
                                    "iconId=0", "description=Desktop client for the Matrix protocol");

            var reg = installer.environmentVariable("SystemRoot") + "\\System32\\reg.exe";
            var key= "HKEY_CLASSES_ROOT\\matrix";
            component.addOperation("Execute", reg, "ADD", key, "/f", "/t", "REG_SZ", "/d", "URL:Matrix Protocol");
            component.addOperation("Execute", reg, "ADD", key, "/f", "/v", "URL Protocol", "/t", "REG_SZ");
            var iconkey = "HKEY_CLASSES_ROOT\\matrix\\DefaultIcon";
            component.addOperation("Execute", reg, "ADD", iconkey, "/f", "/t", "REG_SZ", "/d", "@TargetDir@\\nheko.exe,1");
            component.addOperation("Execute", reg, "ADD", "HKEY_CLASSES_ROOT\\matrix\\shell", "/f");
            component.addOperation("Execute", reg, "ADD", "HKEY_CLASSES_ROOT\\matrix\\shell\\open", "/f");
            var commandkey = "HKEY_CLASSES_ROOT\\matrix\\shell\\open\\command"
            component.addOperation("Execute", reg, "ADD", commandkey, "/f");
            component.addOperation("Execute", reg, "ADD", commandkey, "/f", "/t", "REG_SZ", "/d", "\"@TargetDir@\\nheko.exe\" \"%1\"");
        }
    }
    catch( e )
    {
        print( e );
    }
}
