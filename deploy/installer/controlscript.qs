/**
 * Source: http://stackoverflow.com/questions/21389105/qt-installer-framework-offline-update-how
 */

function Controller()
{
}

Controller.prototype.TargetDirectoryPageCallback = function()
{
    var widget = gui.currentPageWidget();
    widget.TargetDirectoryLineEdit.textChanged.connect( this, Controller.prototype.targetChanged );
    Controller.prototype.targetChanged( widget.TargetDirectoryLineEdit.text );
}

Controller.prototype.targetChanged = function( text )
{
    if( text != "" && installer.fileExists(text + "/components.xml") )
	{
        if( QMessageBox.question("PreviousInstallation", "Previous installation detected", "Do you want to uninstall the previous installation?", QMessageBox.Yes | QMessageBox.No) == QMessageBox.Yes )
		{
            installer.execute( text+"/maintenancetool.exe", new Array("--script", text+"/uninstall.qs") )
		}
    }
}
