#pragma once

#include <Windows.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <random>
#include "Support.h"

namespace tr
{
	class Bus;
	class Route;

	class Station
	{
	private:
		std::mt19937& random;
		static int count;
		int id;
		int timeToWait;
		int arrivingTimeL;
		int arrivingTimeR;
		Bus* clstBus_R;	//	nearest bus going right
		Bus* clstBus_L;	//	nearest bus going left
		Route& route;
	public:
		Station(Route& routeRef, std::mt19937& randomRef);
		void UpdateClstBus(std::vector<Bus*>& busList);
		int TimeToWait();
		int ArrivingTimeL();
		int ArrivingTimeR();
		Bus& GetClstBusL();
		Bus& GetClstBusR();
	};
	/******************************************************************/
	class Segment
	{
	private:
		std::mt19937& random;
		int length;
		int recSpeed;
	public:
		Segment(std::mt19937& randomRef);
		int Length();
		int RecSpeed();
	};
	/******************************************************************/
	class Route
	{
	private:
		std::mt19937& random;
		std::vector<Station> stations;
		std::vector<Segment> segments;
		int count;	//	stations count
		int totalLength;
	public:
		Route(int stationNum, std::mt19937& randomRef);
		Station& operator[](int i); // for getting station
		Segment& operator()(int i); // for getting segment
		int Count();
		int TotalLength();
	};
	/*******************************************************************/
	class Bus
	{
	private:
		std::mt19937& random;
		static int count;
		HANDLE hStopEvent,
			   hUpdateEvent,
			   hThread,
			   hMutex;
		Route& route;
		WCHAR name[12] = L"Автобус 000";
		int status,		//	0 - thread is not running
						//	1 - thread is running, bus is moving
						//	2 - thread is running, bus is waiting
			routeSegment,
			timeToArrive,
			timeToStay,
			locCoord,
			globCoord,
			speed;
		std::pair<int, int> stations;	//	first - departure point, second - dest. point
		void Go();			//	start moving
		void Advance();
		void Stay();		//	stop
		void Wait();		//	wait for 1 tick
		void TurningOff();	//	reset parameters to 0
		static DWORD BusThread(LPVOID pBus); //	thread function

	public:
		Bus(Route& routeRef, std::mt19937& randomRef);
		~Bus();
		bool RunBus();		//	run thread
		bool IsUpdated();
		void TurnOff();
		WCHAR* Name();
		int Status();
		int Speed();
		int LocCoord();		//	coords on current segment
		int GlobCoord();	//	coords on whole route
		int Depart();
		int Dest();
		int TimeToArrive();
		int TimeToStay();
	};
	/******************************************************************************/
	class Traffic
	{
	private:
		std::mt19937 random;
		std::vector<Bus*> roster;
		Route route;
		int busCount;
	public:
		Traffic(int busNum, int stationNum);
		~Traffic();
		Bus& Roster(int index);
		Route& Route();
		void RemoveBus(int id);
		void AddBus();
		void UpdateClstBus();
		void UpdateClstBus(int stationId);
		int BusCount();
	};
}