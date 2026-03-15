modded class SCR_VONEntry : SCR_SelectionMenuEntry
{
    protected bool m_bIsUsable = true;
    protected bool m_bIsActive;
    protected bool m_bIsSelected;
    protected string m_sText;

    // Function to get a unique ID (just an example)
	string GetUniqueId()
	{
		return ToString() + "_" + GetDisplayText();
	}

}