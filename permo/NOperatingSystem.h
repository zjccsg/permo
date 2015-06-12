#ifndef _CNOPERATINGSYSTEM_H_
#define _CNOPERATINGSYSTEM_H_


enum
{
	SBOS_95,
	SBOS_98,
	SBOS_ME,
	SBOS_NT4,
	SBOS_2K,
	SBOS_XP,
	SBOS_XPP, // after xp
};


class CNOperatingSystem
{

public:

	CNOperatingSystem();
	~CNOperatingSystem();

	int GetOS();


};















#endif