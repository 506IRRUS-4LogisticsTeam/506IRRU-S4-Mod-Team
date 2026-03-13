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
			m_pPatientNID.SetReceivingCPR(true);
	}

	override void OnCompartmentLeft()
	{
		if (m_pPatientNID)
			m_pPatientNID.SetReceivingCPR(false);
		super.OnCompartmentLeft();
	}

	SCR_ChimeraCharacter GetPatient() { return m_pPatient; }
}
