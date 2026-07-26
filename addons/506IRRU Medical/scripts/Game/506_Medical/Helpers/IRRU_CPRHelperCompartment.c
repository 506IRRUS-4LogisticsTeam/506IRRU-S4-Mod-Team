class IRRU_CPRHelperCompartmentClass : ACE_AnimationHelperCompartmentClass
{
}

class IRRU_CPRHelperCompartment : ACE_AnimationHelperCompartment
{
	protected IRRU_NoInstantDeathComponent m_pPatientNID;
	protected SCR_ChimeraCharacter m_pPatient;

	void SetPatient(notnull SCR_ChimeraCharacter patient)
	{
		m_pPatient = patient;
		m_pPatientNID = IRRU_NoInstantDeathComponent.Cast(patient.FindComponent(IRRU_NoInstantDeathComponent));
		if (m_pPatientNID)
		{
			m_pPatientNID.IRRU_SetActiveCPRHelper(this);
			m_pPatientNID.SetReceivingCPR(true);
		}
	}

	override void OnCompartmentLeft()
	{
		if (m_pPatientNID && m_pPatientNID.IRRU_GetActiveCPRHelper() == this)
		{
			m_pPatientNID.SetReceivingCPR(false);
			m_pPatientNID.IRRU_SetActiveCPRHelper(null);
		}
		super.OnCompartmentLeft();
	}

	override void Terminate()
	{
		if (!m_bInitDone)
		{
			GetGame().GetCallqueue().Remove(Terminate);
			GetGame().GetCallqueue().CallLater(Terminate, 100);
			return;
		}

		super.Terminate();
	}

	SCR_ChimeraCharacter GetPatient() { return m_pPatient; }
}
