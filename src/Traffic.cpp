#include "Traffic.h"

tr::Traffic::Traffic(int busNum, int stationNum)
	: route(stationNum, random),
	random(std::random_device()())
{
	for (int i = 0; i < busNum; i++)
		roster.push_back(new Bus(route, random));
	busCount = busNum;
}

tr::Traffic::~Traffic()
{
	for (int i = 0; i < roster.size(); i++)
		delete roster[i];
}

tr::Bus& tr::Traffic::Roster(int index)
{
	if (index >= roster.size())
		throw std::out_of_range("tr::Bus& tr::Traffic::GetBus(int index)");
	return *roster[index];
}

tr::Route& tr::Traffic::Route() { return route; }

void tr::Traffic::RemoveBus(int id)
{
	if (id < 0 || id >= roster.size())
		throw std::out_of_range("tr::Traffic::RemoveBus(int id)");
	roster[id]->TurnOff();
	delete roster[id];
	auto it = roster.begin() + id;
	roster.erase(it);
	busCount--;
}

void tr::Traffic::AddBus() 
{ 
	roster.push_back(new Bus(route, random)); 
	busCount++;
}

void tr::Traffic::UpdateClstBus()
{
	for (int i = 0; i < route.Count(); i++)
		route[i].UpdateClstBus(roster);
}

void tr::Traffic::UpdateClstBus(int stationId) { route[stationId].UpdateClstBus(roster); }

int tr::Traffic::BusCount() { return busCount; }


/************************************************************************************/

int tr::Bus::count = 0;
tr::Bus::Bus(Route& routeRef, std::mt19937& randomRef)
	: route(routeRef),
	random(randomRef)
{
	std::unique_ptr<WCHAR> str_id = to_string(100 + count++);
	name[8] = str_id.get()[0];
	name[9] = str_id.get()[1];
	name[10] = str_id.get()[2];
	status = 0;
	bool startLoc = random() % 2;
	if (startLoc)
	{
		routeSegment = 0;
		stations.first = 0;
		stations.second = 1;
		locCoord = 0;
		globCoord = 0;
	}
	else
	{
		routeSegment = route.Count() - 2;
		stations.first = routeSegment + 1;
		stations.second = routeSegment;
		locCoord = route(routeSegment).Length();
		globCoord = route.TotalLength();
	}
	timeToArrive = 0;
	timeToStay = 0;
	speed = 0;

	hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	hUpdateEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
	hThread = INVALID_HANDLE_VALUE;
	hMutex = CreateMutex(NULL, FALSE, L"");
}

tr::Bus::~Bus()
{
	TurnOff();
	CloseHandle(hStopEvent);
	CloseHandle(hUpdateEvent);
	CloseHandle(hThread);
	CloseHandle(hMutex);
}

bool tr::Bus::RunBus()
{
	if (hThread != INVALID_HANDLE_VALUE)
		return false;
	ResetEvent(hStopEvent);
	hThread = CreateThread(NULL, 0, BusThread, this, 0, NULL);
	return true;
}

void tr::Bus::Go()
{
	MutexLocker guard(hMutex);
	status = 1;
	speed = random() % 7 + 4;
	if (stations.first > stations.second)	//	heading to the left
		timeToArrive = route(routeSegment).Length() / speed + 1;
	else
		timeToArrive = (route(routeSegment).Length() - locCoord) / speed + 1;
}

void tr::Bus::Advance()
{
	MutexLocker guard(hMutex);
	int mod = stations.first > stations.second ? -1 : 1;
	locCoord += speed * mod;
	if (locCoord > route(routeSegment).Length())
		globCoord += route(routeSegment).Length() - locCoord + speed * mod;
	else if (locCoord < 0)
		globCoord += speed * mod - locCoord;
	else
		globCoord += speed * mod;
	timeToArrive--;
}

void tr::Bus::Stay()
{
	MutexLocker guard(hMutex);
	status = 2;
	speed = 0;
	timeToStay = route[stations.second].TimeToWait();
	if (stations.second != route.Count() - 1 && stations.second != 0)
	{
		if (stations.first > stations.second)
		{
			stations.first = stations.second--;
			routeSegment--;
			locCoord = route(routeSegment).Length();
		}
		else if (stations.first < stations.second)
		{
			stations.first = stations.second++;
			routeSegment++;
			locCoord = 0;
		}
	}
	else if (stations.second == route.Count() - 1)
	{
		stations.first = stations.second--;
		locCoord = route(routeSegment).Length();
	}
	else if (stations.second == 0)
	{
		stations.first = stations.second++;
		locCoord = 0;
	}
}

void tr::Bus::Wait()
{
	MutexLocker guard(hMutex);
	timeToStay--;
}

void tr::Bus::TurningOff()
{
	MutexLocker guard(hMutex);
	status = 0;
	timeToArrive = 0;
	timeToStay = 0;
	speed = 0;
}

DWORD WINAPI tr::Bus::BusThread(LPVOID pBus)
{
	Bus& bus = *(Bus*)pBus;

	while (true)
	{
		switch (bus.status)
		{
		case 0:	//	thread is not running
		{
			bus.Go();
			break;
		}
		case 1:	//	����
		{
			if (bus.locCoord >= 0 && bus.locCoord <= bus.route(bus.routeSegment).Length())
				bus.Advance();
			else
				bus.Stay();
			break;
		}
		case 2:	//	����
		{
			if (bus.timeToStay > 0)
				bus.Wait();
			else
				bus.Go();
			break;
		}
		default:
			break;
		}
		SetEvent(bus.hUpdateEvent);
		if (WaitForSingleObject(bus.hStopEvent, 100) != WAIT_TIMEOUT)
			break;
	}

	bus.TurningOff();
	return 0;
}

bool tr::Bus::IsUpdated()
{
	if (WaitForSingleObject(hUpdateEvent, 0) == WAIT_OBJECT_0)
	{
		ResetEvent(hUpdateEvent);
		return true;
	}
	return false;
}

void tr::Bus::TurnOff()
{
	if (hThread == INVALID_HANDLE_VALUE) return;
	SetEvent(hStopEvent);
	if (WaitForSingleObject(hThread, 5000) != WAIT_OBJECT_0)
		TerminateThread(hThread, 1);
	CloseHandle(hThread);
	hThread = INVALID_HANDLE_VALUE;
	SetEvent(hUpdateEvent);
}

WCHAR* tr::Bus::Name() 
{
	MutexLocker guard(hMutex);
	return name;
}

int tr::Bus::Status() 
{ 
	MutexLocker guard(hMutex);
	return status; 
}

int tr::Bus::Speed()
{
	MutexLocker guard(hMutex);
	return speed;
}

int tr::Bus::LocCoord()
{
	MutexLocker guard(hMutex);
	if (locCoord > route(routeSegment).Length())
		return route(routeSegment).Length();
	else if (locCoord < 0)
		return 0;
	return locCoord;
}

int tr::Bus::GlobCoord()
{
	MutexLocker guard(hMutex);
	return globCoord;
}

int tr::Bus::Depart()
{
	MutexLocker guard(hMutex);
	return stations.first;
}

int tr::Bus::Dest()
{
	MutexLocker guard(hMutex);
	return stations.second;
}

int tr::Bus::TimeToArrive()
{
	MutexLocker guard(hMutex);
	return timeToArrive;
}

int tr::Bus::TimeToStay()
{
	MutexLocker guard(hMutex);
	return timeToStay;
}

/**************************************************************************************/

tr::Route::Route(int stationNum, std::mt19937& randomRef) : random(randomRef)
{
	count = stationNum;
	for (int i = 0; i < count; i++)
		stations.emplace_back(Station(*this, randomRef));
	for (int i = 0; i < count; i++)
		segments.emplace_back(Segment(randomRef));
	totalLength = 0;
	for (int i = 0; i < count - 1; i++)
		totalLength += segments[i].Length();
}

tr::Station& tr::Route::operator[](int i)
{
	if (i < 0 || i >= stations.size())
		throw std::out_of_range("tr::Station& tr::Route::operator[](int i)");
	return this->stations[i];
}

tr::Segment& tr::Route::operator()(int i)
{
	if (i < 0 || i >= segments.size())
		throw std::out_of_range("tr::Segment& tr::Route::operator()(int i)");
	return this->segments[i];
}

int tr::Route::Count() { return count; }

int tr::Route::TotalLength() { return totalLength; }

/************************************************************************************/
int tr::Station::count = 0;

tr::Station::Station(Route& routeRef, std::mt19937& randomRef) : route(routeRef), random(randomRef)
{
	id = count++;
	timeToWait = (random() % 7 + 4) * 10;
	arrivingTimeL = 0;
	arrivingTimeR = 0;
	clstBus_L = nullptr;
	clstBus_R = nullptr;
}

void tr::Station::UpdateClstBus(std::vector<Bus*>& busList)
{
	if (busList.size() == 0)
		int i = 0;
	int minTimeLeft = INT_MAX,
		minTimeRight = INT_MAX,
		busIdL = 0,
		busIdR = 0,
		timeFromStart = 0,
		timeFromEnd = 0;
	for (int i = 0; i < id; i++)
	{
		timeFromStart += route(i).Length() / route(i).RecSpeed();
		timeFromStart += route[i].TimeToWait();
	}
	timeFromStart = timeFromStart * 2 - route[0].TimeToWait() + this->timeToWait;
	for (int i = id; i < route.Count() - 2; i++)
	{
		timeFromEnd += route(i).Length() / route(i).RecSpeed();
		timeFromEnd += route[i + 1].TimeToWait();
	}
	timeFromEnd = timeFromEnd * 2 - route[route.Count() - 1].TimeToWait() + this->timeToWait;

	for (int i = 0; i < busList.size(); i++)
	{
		if (busList[i]->Status() == 0)
			continue;
		int depart = busList[i]->Depart(),
			dest = busList[i]->Dest(),
			arrTime = 0;

		if (depart < dest)	//	���� ������
		{
			if (dest <= id)	//	��������� ����� �� �������
			{
				for (int j = id - 1; j > depart; j--)
				{
					arrTime += route(j).Length() / route(j).RecSpeed();
					arrTime += route[j].TimeToWait();
				}
				if (busList[i]->Status() == 1)
					arrTime += (route(depart).Length() - busList[i]->LocCoord()) / busList[i]->Speed();
				else if (busList[i]->Status() == 2)
					arrTime += busList[i]->TimeToStay() + route(depart).Length() / route(depart).RecSpeed();

				if (arrTime < minTimeLeft)
				{
					minTimeLeft = arrTime;
					busIdL = i;
				}
				if (arrTime + timeFromEnd < minTimeRight)
				{
					minTimeRight = arrTime + timeFromEnd;
					busIdR = i;
				}
			}
			else									//	��������� ������ �� �������
			{
				for (int j = dest; j < route.Count() - 1; j++)
				{
					arrTime += route(j).Length() / route(j).RecSpeed();
					arrTime += route[j].TimeToWait();
				}
				if (busList[i]->Status() == 1)
					arrTime += (route(depart).Length() - busList[i]->LocCoord()) / busList[i]->Speed();
				else if (busList[i]->Status() == 2)
					arrTime += busList[i]->TimeToStay() + route(depart).Length() / route(depart).RecSpeed();
				for (int j = route.Count() - 1; j > id; j--)
				{
					arrTime += route[j].TimeToWait();
					arrTime += route(j - 1).Length() / route(j - 1).RecSpeed();
				}

				if (arrTime + timeFromStart < minTimeLeft)
				{
					minTimeLeft = arrTime + timeFromStart;
					busIdL = i;
				}
				if (arrTime < minTimeRight)
				{
					minTimeRight = arrTime;
					busIdR = i;
				}
			}
		}
		else if (depart > dest)	//	���� �����
		{
			if (dest >= id)		// ��������� ������ �� �������
			{
				for (int j = dest; j > id; j--)
				{
					arrTime += route(j - 1).Length() / route(j - 1).RecSpeed();
					arrTime += route[j].TimeToWait();
				}
				if (busList[i]->Status() == 1)
					arrTime += busList[i]->LocCoord() / busList[i]->Speed();
				else if (busList[i]->Status() == 2)
					arrTime += busList[i]->TimeToStay() + route(dest).Length() / route(dest).RecSpeed();

				if (arrTime + timeFromStart < minTimeLeft)
				{
					minTimeLeft = arrTime + timeFromStart;
					busIdL = i;
				}
				if (arrTime < minTimeRight)
				{
					minTimeRight = arrTime;
					busIdR = i;
				}
			}
			else				//	��������� ����� �� �������
			{
				for (int j = dest; j > 0; j--)
				{
					arrTime += route(j - 1).Length() / route(j - 1).RecSpeed();
					arrTime += route[j].TimeToWait();
				}
				if (busList[i]->Status() == 1)
					arrTime += busList[i]->LocCoord() / busList[i]->Speed();
				else if (busList[i]->Status() == 2)
					arrTime += busList[i]->TimeToStay() + route(dest).Length() / route(dest).RecSpeed();
				for (int j = 0; j < id; j++)
				{
					arrTime += route[j].TimeToWait();
					arrTime += route(j).Length() / route(j).RecSpeed();
				}

				if (arrTime < minTimeLeft)
				{
					minTimeLeft = arrTime;
					busIdL = i;
				}
				if (arrTime + timeFromEnd < minTimeRight)
				{
					minTimeRight = arrTime + timeFromEnd;
					busIdR = i;
				}
			}
		}
	}
	clstBus_L = busList[busIdL];
	arrivingTimeL = minTimeLeft == INT_MAX ? 0 : minTimeLeft;
	clstBus_R = busList[busIdR];
	arrivingTimeR = minTimeRight == INT_MAX ? 0 : minTimeRight;
}

int tr::Station::TimeToWait()	{ return timeToWait; }

int tr::Station::ArrivingTimeL() { return arrivingTimeL; }

int tr::Station::ArrivingTimeR() { return arrivingTimeR; }

tr::Bus& tr::Station::GetClstBusL() { return *clstBus_L; }

tr::Bus& tr::Station::GetClstBusR() { return *clstBus_R; }

/*************************************************************************************/

tr::Segment::Segment(std::mt19937& randomRef) : random(randomRef)
{
	length = (random() % 61 + 20) * 10;
	recSpeed = random() % 7 + 4;
}

int tr::Segment::Length() { return length; }

int tr::Segment::RecSpeed() { return recSpeed; }