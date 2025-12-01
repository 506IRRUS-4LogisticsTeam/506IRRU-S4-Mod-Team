// SHS RPL
class SHS_RplComponentClass: RplComponentClass
{
}

class SHS_RplComponent: RplComponent
{
	void TrySetRplParent(RplNode newParent)
	{
		GetNode().SetParent(newParent);
	}
}
