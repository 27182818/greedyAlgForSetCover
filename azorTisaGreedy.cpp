#include <iostream>
#include <fstream>
#include <list>
#define zoneTypes 8
#define numZones 20
#define numDays 5
#define numHours 5
#define numRequests 1024
#define firstNightHour 3
using namespace std;

enum howToParseRequests{tayesetNumber, zoneType, daytime, sunday, monday, tuesday, wednesday, thursday};
enum azorTisaTypes{kravAvir, kravEW, kravBombing, helicopterBombing, helicopterEscort, helicopterEvac, UAVEscort, UAVBombing};
class Booking {
  public:
    int reqNum;
    int flightZone;
    int day;
    int hour;
    int tayeset;
    Booking(int requestNum, int flightZoneNum, int dayNum, int hourNum, int tayesetNum = 0) {
      reqNum = requestNum;
      flightZone = flightZoneNum;
      day = dayNum;
      hour = hourNum;
      tayeset = tayesetNum;
    }
    Booking() {}
};

bool compareBooking(Booking a, Booking b){
  return a.tayeset < b.tayeset;
}

list <Booking> success;
list <Booking> failed;
bool flyZones[zoneTypes][numZones];
bool bookings[numHours][numDays][numZones];
int requests[zoneTypes][numRequests];
int bookedReqs = 0;
int shiftedBookings = 0;
int secondAttemptBookings = 0;

void csv2array(string csvname, int xLimit, int yLimit, void( * arrayFunc)(int, int, char)) {
  ifstream file(csvname.c_str());
  string line;
  int lineNumber = 0;
  while (file.good()) {
    getline(file, line);
    int y = 0;
    for (int x = 0; x < xLimit; x++) {
      while (line[y] < '0' || line[y] > '9') y++;
      ( * arrayFunc)(x, lineNumber, line[y++]);
    }
    lineNumber++;
    if (lineNumber > yLimit - 1)
      break;
  }
}

void flyzoneArray(int x, int y, char c) {
  flyZones[x][y] = (c == '1');
}

void requestsArray(int x, int y, char c) {
  requests[x][y] = c - '0';
}

void blank(int a) {}
void blank(int a, int b, int c, int d) {}
void blank(int a, int b, int c, Booking d) {}

void greedySuccess(int hour, int day, int flightZone, int req) {
  bookings[hour][day][flightZone] = false;
  //cout << "booked request " << req << " for tayeset " << requests[0][req] << " for day " << day + 1 << " for hour " << hour + 1 << " at azor " << flightZone + 1 << endl;
  success.push_front( * new Booking(req, flightZone, day, hour, requests[0][req]));
  bookedReqs++;
}

void greedyFail(int req) {
  //cout << "failed to book request " << req << endl;
  failed.push_front( * new Booking(req, 0, 0, 0, requests[0][req]));
}

void shiftDownSuccess(int hour, int day, int flightZone, Booking booking) {
  bookings[hour][day][flightZone] = false;
  bookings[booking.hour][booking.day][booking.flightZone] = true;
  cout<<"shifted booking "<<booking.reqNum<<" for tayeset "<< booking.tayeset<<" to azor "<<flightZone<<" for day "<<day<<" for hour "<<hour<<endl;
  shiftedBookings++;
}

void secondAttemptSuccess(int hour, int day, int flightZone, int req) {
  bookings[hour][day][flightZone] = false;
  cout << "On the second attempt, booked request " << req << " for tayeset " << requests[0][req] << " for day " << day + 1 << " for hour " << hour + 1 << " at azor " << flightZone + 1 << endl;
  secondAttemptBookings++;
}

void iterateOverTimeslots(int req, void( * successFunc)(int, int, int, int), void( * failFunc)(int) = blank, Booking booking = Booking(), void(successFuncWithBookingParameter)(int, int, int, Booking) = blank) {
  bool isBooked = false;
  for (int flightZone = 0; flightZone < numZones; flightZone++) {
    int type = requests[zoneType][req] - 1;
    for (int day = 0; day < numDays; day++) {
      int starthour, endhour;
      if (requests[daytime][req]) {
        starthour = 0;
        endhour = firstNightHour;
      } else {
        starthour = firstNightHour;
        endhour = numHours;
      }
      for (int hour = starthour; hour < endhour; hour++) {
        if (requests[sunday + day][req] && flyZones[type][flightZone] && bookings[hour][day][flightZone]) {
          ( * successFunc)(hour, day, flightZone, req);
          ( * successFuncWithBookingParameter)(hour, day, flightZone, booking);
          isBooked = true;
          break;
        }
      }
      if (isBooked) {
        break;
      }
    }
    if (isBooked) {
      break;
    }
  }
  if (isBooked) {
    isBooked = false;
  } else {
    ( * failFunc)(req);
  }
}
void shiftAndRebook(){
  //now, try to shift down all requests, and then try again to add new requests:
  for (list <Booking>::const_iterator iter = success.begin(); iter != success.end(); iter++) {
    iterateOverTimeslots(iter->reqNum, blank, blank, * iter, shiftDownSuccess);
  }
  //now, see if we can add new requests
  for (list <Booking>::const_iterator iter = failed.begin(); iter != failed.end(); iter++) {
    iterateOverTimeslots(iter->reqNum, secondAttemptSuccess);
  }
}

void visualizeBookings(){
  for (int y = 0; y < numHours; y++) {
    for (int x = 0; x < numDays; x++) {
      for(int z = 0; z < numZones; z++){
        cout << bookings[x][y][z];
      }
      cout<<endl;
    }
    cout << endl;
  }
  cout << endl;     
}

int main() {
  for (int x = 0; x < numHours; x++) {
    for (int y = 0; y < numDays; y++) {
      for (int z = 0; z < numZones; z++) {
        bookings[x][y][z] = true;
      }
    }
  }

  string csvname = "azoreiTisa.csv";
  //cout<<"input azorei tisa file"<<endl;
  //cin>>csvname;
  csv2array(csvname, zoneTypes, numZones, flyzoneArray);

  //cout<<"input request file"<<endl;
  //cin>>csvname;
  csvname = "requests.csv";
  csv2array(csvname, zoneTypes, numRequests, requestsArray);

  //run greedy algorithm:
  //for each request, book first available azor tisa
  for (int req = 0; req < numRequests; req++) {
    iterateOverTimeslots(req, greedySuccess, greedyFail);
  }
  //shiftAndRebook();
  cout << "booked " << bookedReqs<<":"<< endl;
  //cout << "switched " << shiftedBookings << endl;
  //cout << "on the second attempt, booked " << secondAttemptBookings << " new request(s)" << endl;
  //visualizeBookings();
  success.sort(compareBooking);
  for (list <Booking>::const_iterator iter = success.begin(); iter != success.end(); iter++) {
    cout<<"booked tayeset number "<<iter->tayeset<<" for azor "<<iter->flightZone<<" for day "<<iter->day<<" for hour "<<iter->hour<<"; request number "<<iter->reqNum<<endl;
  }
  
  system("pause");
}
