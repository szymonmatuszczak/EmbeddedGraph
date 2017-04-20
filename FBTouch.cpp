using namespace std;

const unsigned char CANNOT_OPEN_TOUCH_DEVICE = 6;

class TouchField
{
	public:
		unsigned int Xmin;
		unsigned int Xmax;
		unsigned int Ymin;
		unsigned int Ymax;
		TouchField(unsigned int XminL=0, unsigned int XmaxL=0, unsigned int YminL=0, unsigned int YmaxL=0)
		{
			Xmin = XminL;
			Xmax = XmaxL;
			Ymin = YminL;
			Ymax = YmaxL;
		}
};

class FBTouch
{
	private:
		unsigned long long int lastTouchEventMSTime = 0;
		int InputHandler = -1;
		string InputDeviceName;
		vector<function<void(unsigned int, unsigned int)>> eventListeners;
		vector<pair<TouchField, function<void(unsigned int, unsigned int)>>> touchFields;
		void die(const unsigned char error)
		{
			cout << "Cannot initialize input device, error code: " << error << endl;
			exit(1);
		}
		void internalTouch(unsigned int x, unsigned int y)
		{
			if(GetCurrentMSTime() >= lastTouchEventMSTime+150)
			{
				for(vector<pair<TouchField, function<void(unsigned int, unsigned int)>>>::iterator it=touchFields.begin();it!=touchFields.end();it++) if(x >= it->first.Xmin && x <= it->first.Xmax && y >= it->first.Ymin && y <= it->first.Ymax) it->second(x, y);
				for(vector<function<void(unsigned int, unsigned int)>>::iterator it=eventListeners.begin();it!=eventListeners.end();it++) (*it)(x, y);
			}
			lastTouchEventMSTime = GetCurrentMSTime();
		}
	public:
		thread touchThread;
		void waitForTouch()
		{
			touchThread.join();
		}
		void removeTouchEventListener(unsigned int id)
		{
			eventListeners.erase(eventListeners.begin()+id);
		}
		unsigned int addTouchEventListener(function<void(unsigned int, unsigned int)> react)
		{
			eventListeners.push_back(react);
			return eventListeners.size()-1;
		}
		void removeTouchField(unsigned int id)
		{
			touchFields.erase(touchFields.begin()+id);
		}
		unsigned int addTouchField(TouchField toAdd, function<void(unsigned int, unsigned int)> listener)
		{
			touchFields.push_back(make_pair(toAdd, listener));
			return touchFields.size()-1;
		}
		FBTouch(const string InputDeviceNameL)
		{
			InputDeviceName = InputDeviceNameL;
			InputHandler = open(InputDeviceName.c_str(), O_RDONLY);
			if(InputHandler == -1) die(CANNOT_OPEN_TOUCH_DEVICE);
			touchThread = thread([this, InputHandler=InputHandler]()->void
			{
				struct input_event ev;
				const float scaleXvalue = ((float)3932-300)/320;
				const float scaleYvalue = ((float)3801-294)/480;
				for(;;)
				{
					unsigned int Xi = 0;
					unsigned int Yi = 0;
					unsigned int Xpos = 0;
					unsigned int Ypos = 0;
					while(Xi<1||Yi<1)
					{
						ssize_t size = read(InputHandler, &ev, sizeof(struct input_event));
						if(ev.type == EV_ABS)
						{
							if(ev.code == ABS_X)
							{
								Xpos = ev.value;
								Xi++;
							}
							if(ev.code == ABS_Y)
							{
								Ypos = ev.value;
								Yi++;
							}
						}
					}
					const float Xb = (Xpos/scaleXvalue)-30;
					const float Yb = (Ypos/scaleYvalue)-45;
					const unsigned int X = (unsigned int)round(Xb);
					const unsigned int Y = (unsigned int)round(Yb);
					internalTouch(X, Y);
				}
			});
		}
};