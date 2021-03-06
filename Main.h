LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HWND InitWindow(HINSTANCE h_instance, LPCWSTR window_title, UINT width, UINT height, bool full_screen,
                int background_color);

struct Timer {
  double timer_frequency = 0.0;
  long long last_frame_time = 0;
  long long last_second = 0;
  double frame_delta = 0;
  int fps = 0;


  Timer() {
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);

    // seconds
    //timer_frequency = double(li.QuadPart);

    // milliseconds
    timer_frequency = double(li.QuadPart) / 1000.0;

    // microseconds
    //timer_frequency = double(li.QuadPart) / 1000000.0;

    QueryPerformanceCounter(&li);
    last_frame_time = li.QuadPart;
  }


  // Call this once per frame
  double GetFrameDelta() {
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    frame_delta = double(li.QuadPart - last_frame_time) / timer_frequency;
    if (frame_delta > 0)
      fps = 1000 / frame_delta;
    last_frame_time = li.QuadPart;
    return frame_delta;
  }
};

Timer m_timer = Timer();

int m_vertCount = 3;
LONG m_width = DXGE_WINDOW_DEFAULT_WIDTH;;
LONG m_height = DXGE_WINDOW_DEFAULT_HEIGHT;

bool m_splashscreen = true;
