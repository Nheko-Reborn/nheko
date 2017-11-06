function Controller()
{
}

Controller.prototype.IntroductionPageCallback = function()
{
    gui.clickButton( buttons.NextButton );
}

Controller.prototype.ReadyForInstallationPageCallback = function()
{
    gui.clickButton( buttons.CommitButton );
}

Controller.prototype.FinishedPageCallback = function()
{
    gui.clickButton( buttons.FinishButton );
}
