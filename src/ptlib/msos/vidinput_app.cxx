/* vidinput_app.cxx
 *
 *
 * Application Implementation for the PTLib Project.
 *
 * Copyright (c) 2007 ISVO (Asia) Pte Ltd. All Rights Reserved.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 *
 *
 * Contributor(s): Craig Southeren, Post Increment (C) 2008
 *
 * $Revision$
 * $Author$
 * $Date$
 */


#include <ptlib.h>


#if P_APPSHARE

#include <ptlib/vconvert.h>
#include <ptclib/delaychan.h>


static const char TopWindowPrefix[] = "TopWindow:";
static const PINDEX TopWindowPrefixLen = sizeof(TopWindowPrefix)-1;


#ifdef _MSC_VER
  #pragma message("Application sharing video support enabled")
#endif


//////////////////////////////////////////////////////////////////////
// Video Input device

/**This class defines a video input device for Application sharing on Windows.
 */

class PVideoInputDevice_Application : public PVideoInputDevice
{
  PCLASSINFO(PVideoInputDevice_Application, PVideoInputDevice);

  public:
    /** Create a new video input device.
     */
    PVideoInputDevice_Application();

    /**Close the video input device on destruction.
      */
    ~PVideoInputDevice_Application();

    /** Is the device a camera, and obtain video
     */
    static PStringArray GetInputDeviceNames();

    virtual PStringArray GetDeviceNames() const;

    /**Retrieve a list of Device Capabilities
    */
    static PBoolean GetDeviceCapabilities(
      const PString & deviceName, ///< Name of device
      Capabilities * caps         ///< List of supported capabilities
    );

    /**Open the device given the device name.
      */
    virtual PBoolean Open(
      const PString & DeviceName,   ///< Device name to open
      PBoolean startImmediate = TRUE    ///< Immediately start device
    );

    /**Determine if the device is currently open.
      */
    virtual PBoolean IsOpen();

    /**Close the device.
      */
    virtual PBoolean Close();

    /**Start the video device I/O.
      */
    virtual PBoolean Start();

    /**Stop the video device I/O capture.
      */
    virtual PBoolean Stop();

    /**Determine if the video device I/O capture is in progress.
      */
    virtual PBoolean IsCapturing();

    /**Set the colour format to be used.
       Note that this function does not do any conversion. If it returns TRUE
       then the video device does the colour format in native mode.

       To utilise an internal converter use the SetColourFormatConverter()
       function.

       Default behaviour sets the value of the colourFormat variable and then
       returns TRUE.
    */
    virtual PBoolean SetColourFormat(
      const PString & colourFormat ///< New colour format for device.
    );

    /**Get the maximum frame size in bytes.

       Note a particular device may be able to provide variable length
       frames (eg motion JPEG) so will be the maximum size of all frames.
      */
    virtual PINDEX GetMaxFrameBytes();

    /**Grab a frame, after a delay as specified by the frame rate.
      */
    virtual PBoolean GetFrameData(
      BYTE * buffer,                 ///< Buffer to receive frame
      PINDEX * bytesReturned         ///< OPtional bytes returned.
    );

    /**Grab a frame. Do not delay according to the current frame rate parameter.
      */
    virtual PBoolean GetFrameDataNoDelay(
      BYTE * buffer,                 ///< Buffer to receive frame
      PINDEX * bytesReturned         ///< OPtional bytes returned.
    );

    /**Set the Handle of the window you wish to capture
      */
    void AttachCaptureWindow(
      HWND hwnd,            ///< Handle of the window to capture
      bool client = TRUE    ///< Only capture Client area and not caption
    );

  protected:
    bool GetWindowBitmap(BITMAP & bitmapInfo, BYTE * pixels = NULL, bool useTemp = false);

    HWND       m_hWnd;               ///< Handle of Window to Capture
    bool       m_client;             ///< Capture the client area only
    PBYTEArray m_tempPixelBuffer;    ///< used for storing temporary version of bitmap during convertion

    PMutex         m_lastFrameMutex; ///< Frame Grab Mutex
    PAdaptiveDelay m_grabDelay;      ///< Frame Grab delay
};



//////////////////////////////////////////////////////////////////////////////////////

class PVideoInputDevice_Application_PluginServiceDescriptor : public PDevicePluginServiceDescriptor
{
  public:
    virtual PObject *    CreateInstance(int /*userData*/) const { return new PVideoInputDevice_Application; }

    virtual PStringArray GetDeviceNames(int /*userData*/) const { return PVideoInputDevice_Application::GetInputDeviceNames(); }

    virtual bool         GetDeviceCapabilities(const PString & deviceName, void * caps) const
    {
      return PVideoInputDevice_Application::GetDeviceCapabilities(deviceName, (PVideoInputDevice::Capabilities *)caps);
    }

    virtual bool ValidateDeviceName(const PString & deviceName, int /*userData*/) const
    {
      return (deviceName *= "Desktop") ||
            ((deviceName.Left(TopWindowPrefixLen) *= TopWindowPrefix) && (FindWindow(NULL, deviceName.Mid(TopWindowPrefixLen)) != NULL));
    }
} PVideoInputDevice_Application_descriptor;

PCREATE_PLUGIN(Application, PVideoInputDevice, &PVideoInputDevice_Application_descriptor);


//////////////////////////////////////////////////////////////////////////////////////////////////
// Input device

PVideoInputDevice_Application::PVideoInputDevice_Application()
  : m_hWnd(NULL)
  , m_client(false)
{
  SetColourFormat("BGR32");
  SetFrameRate(5);
}


PVideoInputDevice_Application::~PVideoInputDevice_Application()
{
  Close();
}


PBoolean PVideoInputDevice_Application::Close()
{
  if (!IsOpen())
    return false;

  m_hWnd = NULL;
  return true;
}


PStringArray PVideoInputDevice_Application::GetDeviceNames() const
{ 
  return GetInputDeviceNames(); 
}


static BOOL CALLBACK AddWindowName(HWND hWnd, LPARAM userData)
{
  CHAR name[200];
  if (IsWindowVisible(hWnd) && GetWindowText(hWnd, name, sizeof(name))) {
    PStringArray * names = (PStringArray *)userData;
    names->AppendString(PString(TopWindowPrefix) + name);
  }
  return TRUE;
}


PStringArray PVideoInputDevice_Application::GetInputDeviceNames()
{
  PStringArray names;

  names += "Desktop";

  ::EnumWindows(AddWindowName, (LPARAM)&names);

  return names;
}


PBoolean PVideoInputDevice_Application::GetDeviceCapabilities(const PString & /*deviceName*/, Capabilities * /*caps*/)  
{ 
  return false; 
}


PBoolean PVideoInputDevice_Application::Open(const PString & deviceName, PBoolean /*startImmediate*/)
{
  Close();

  m_client = false;

  if (deviceName.Left(TopWindowPrefixLen) *= TopWindowPrefix)
    m_hWnd = FindWindow(NULL, deviceName.Mid(TopWindowPrefixLen));
  else if (deviceName *= "desktop")
    m_hWnd = GetDesktopWindow();

  if (m_hWnd == NULL) {
    PTRACE(4,"AppInput/tCannot open specified window");
    return false;
  }

  BITMAP bitmap;
  if (GetWindowBitmap(bitmap))
    return PVideoDevice::SetColourFormat(bitmap.bmBitsPixel == 32 ? "BGR32" : "BGR24");

  m_hWnd = NULL;
  return false;
}


PBoolean PVideoInputDevice_Application::IsOpen()
{
  return m_hWnd != NULL;
}


PBoolean PVideoInputDevice_Application::Start()
{
  return true;
}


PBoolean PVideoInputDevice_Application::Stop()
{
  return true;
}


PBoolean PVideoInputDevice_Application::IsCapturing()
{
  return IsOpen();
}


PBoolean PVideoInputDevice_Application::SetColourFormat(const PString & colourFormat)
{
  BITMAP bitmap;
  if (GetWindowBitmap(bitmap)) {
    if (!(colourFormat *= (bitmap.bmBitsPixel == 32 ? "BGR32" : "BGR24")))
      return false;
  }
  else {
    if (!((colourFormat *= "BGR32") || (colourFormat *= "BGR24")))
      return false;
  }

  return PVideoDevice::SetColourFormat(colourFormat);
}


PINDEX PVideoInputDevice_Application::GetMaxFrameBytes()
{
  PINDEX size;
  BITMAP bitmap;
  if (GetWindowBitmap(bitmap))
    size = bitmap.bmHeight*bitmap.bmWidthBytes;
  else
    size = CalculateFrameBytes(frameWidth, frameHeight, colourFormat);

  return GetMaxFrameBytesConverted(size);
}


PBoolean PVideoInputDevice_Application::GetFrameData(BYTE * buffer, PINDEX * bytesReturned)
{
  m_grabDelay.Delay(1000/GetFrameRate());
  return GetFrameDataNoDelay(buffer, bytesReturned);
}


PBoolean PVideoInputDevice_Application::GetFrameDataNoDelay(BYTE * buffer, PINDEX * bytesReturned)
{
  PWaitAndSignal mutex(m_lastFrameMutex);

  BITMAP bitmap;
  if (converter == NULL) {
    if (GetWindowBitmap(bitmap, buffer)) {
      if (bytesReturned != NULL)
        *bytesReturned = bitmap.bmHeight*bitmap.bmWidthBytes;
      return true;
    }
  }
  else {
    if (GetWindowBitmap(bitmap, NULL, true)) {
      converter->SetSrcFrameSize(frameWidth, frameHeight);
      if (converter->Convert(m_tempPixelBuffer, buffer, bytesReturned))
        return true;
    }

    PTRACE(2, "AppInput\tConverter failed");
  }

  return false;
}


void PVideoInputDevice_Application::AttachCaptureWindow(HWND hWnd, bool client)
{
  m_hWnd = hWnd;
  m_client = client;
}


struct PWrapHDC
{
  HDC m_handle;
  PWrapHDC(HDC h) : m_handle(h) { }
  ~PWrapHDC() { DeleteDC(m_handle); }
  operator HDC() const { return m_handle; }
};


struct PWrapHGDIOBJ
{
  HGDIOBJ m_handle;
  PWrapHGDIOBJ(HGDIOBJ h) : m_handle(h) { }
  ~PWrapHGDIOBJ() { DeleteObject(m_handle); }
  operator HGDIOBJ() const { return m_handle; }
  operator HBITMAP() const { return (HBITMAP)m_handle; }
};


bool PVideoInputDevice_Application::GetWindowBitmap(BITMAP & bitmap, BYTE * pixels, bool useTemp)
{
  if (m_hWnd == NULL) {
    PTRACE(2, "AppInput\tNo window selected.");
    return false;
  }

  RECT rect;

  // Get the client area of the window
  if (m_client) {

    ::GetClientRect(m_hWnd, &rect);

    POINT pt1;
    pt1.x = rect.left;
    pt1.y = rect.top;
    ::ClientToScreen(m_hWnd, &pt1);

    POINT pt2;
    pt2.x = rect.right;             
    pt2.y = rect.bottom;             
    ::ClientToScreen(m_hWnd, &pt2);

    rect.left   = pt1.x;             
    rect.top    = pt1.y;             
    rect.right  = pt2.x;             
    rect.bottom = pt2.y;  
  }
  else
  {
    ::GetWindowRect(m_hWnd, &rect);
  }

  if (IsRectEmpty(&rect)) {
    PTRACE(2, "AppInput\tZero sized window");
    return false;
  }

  // get width and height of grab region
  unsigned width  = rect.right - rect.left;
  unsigned height = rect.bottom - rect.top;

  // create a DC for the screen and create
  PWrapHDC hScrDC = CreateDC("DISPLAY", NULL, NULL, NULL);

  // Get the frame size;
  unsigned framewidth, frameheight;
  GetFrameSize(framewidth, frameheight);

   // create a bitmap compatible with the screen DC
  PWrapHGDIOBJ hBitMap = CreateCompatibleBitmap(hScrDC, framewidth, frameheight);

  // a memory DC compatible to screen DC
  PWrapHDC hMemDC = CreateCompatibleDC(hScrDC);

  // select new bitmap into memory DC
  SelectObject(hMemDC, hBitMap);

  // bitblt or stretchblt screen DC to memory DC depending on frame size
  if (width == framewidth && height == frameheight)
    BitBlt(hMemDC, 0, 0, width, height, hScrDC, rect.left, rect.top, SRCCOPY);
  else
    StretchBlt(hMemDC, 0, 0, framewidth, frameheight, hScrDC, rect.left, rect.top, width, height, SRCCOPY);

  // get the bitmap information
  if (GetObject(hBitMap, sizeof(BITMAP), (LPSTR)&bitmap) == 0) {
    PTRACE(2, "AppInput\tCould not get bitmap information");
    return false;
  }

  if (pixels != NULL && ((unsigned)bitmap.bmWidth > frameWidth || (unsigned)bitmap.bmHeight > frameHeight)) {
    PTRACE(2, "AppInput\tWindow increased in size, buffer may not be large enough.");
    return true;
  }

  SetFrameSize(bitmap.bmWidth, bitmap.bmHeight);

  // create a BITMAPINFO with enough room for the pixel data
  unsigned bitmapInfoSize = sizeof(BITMAPINFOHEADER);
  if (bitmap.bmBitsPixel <= 8)
#ifdef _WIN64
    bitmapInfoSize += (1i64 << bitmap.bmBitsPixel) * sizeof(RGBQUAD);
#else
    bitmapInfoSize += (1 << bitmap.bmBitsPixel) * sizeof(RGBQUAD);
#endif

  LPBITMAPINFO bitmapInfo = (LPBITMAPINFO)_alloca(bitmapInfoSize);

  BITMAPINFOHEADER & bi = bitmapInfo->bmiHeader;
  memset(&bi, 0, sizeof(bi));
  bi.biSize        = sizeof(BITMAPINFOHEADER);
  bi.biWidth       =  bitmap.bmWidth;
  bi.biHeight      = -bitmap.bmHeight;
  bi.biBitCount    =  bitmap.bmBitsPixel;
  bi.biPlanes      = 1;
  bi.biCompression = BI_RGB;

  if (useTemp)
    pixels = m_tempPixelBuffer.GetPointer(bitmap.bmHeight*bitmap.bmWidthBytes);

  // get the pixel data
  if (GetDIBits(hMemDC,
                hBitMap,
                0, bitmap.bmHeight,
                pixels,
                bitmapInfo,
                DIB_RGB_COLORS) == 0) {
    PTRACE(2, "AppInput\tFailed to get bitmap image from window");
    return false;
  }

  return true;
}


#else

  #ifdef _MSC_VER
    #pragma message("Application sharing video support DISABLED")
  #endif

#endif  // P_APPSHARE
