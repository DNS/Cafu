/*********************************************************************NVMH3****
Path:  E:\Documents and Settings\sdomine\My Documents\CGFX_Beta_Runtime\include
File:  ICgFX.h

Copyright NVIDIA Corporation 2002
TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, THIS SOFTWARE IS PROVIDED
*AS IS* AND NVIDIA AND ITS SUPPLIERS DISCLAIM ALL WARRANTIES, EITHER EXPRESS
OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL NVIDIA OR ITS SUPPLIERS
BE LIABLE FOR ANY SPECIAL, INCIDENTAL, INDIRECT, OR CONSEQUENTIAL DAMAGES
WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS,
BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS)
ARISING OUT OF THE USE OF OR INABILITY TO USE THIS SOFTWARE, EVEN IF NVIDIA HAS
BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.


Comments:




******************************************************************************/


#ifndef __I_CGFX_H
#define __I_CGFX_H

#include "CgFX/cgfx_stddefs.h"

class IDeviceStateMgr;

namespace ICgFX 
{
    typedef HRESULT CreateFunction( IDeviceStateMgr** pMgr, const DWORD& theHandle );
    bool RegisterDeviceCreateFunction(const char* pName, CreateFunction* pFunction );

    HRESULT SetDeviceType(const char* pDeviceName, const DWORD& deviceHandle);
    HRESULT FreeDevice(const char* pDeviceName, const DWORD& deviceHandle);

    typedef void (* Callback)( void* pData, const char* const pString );
    HRESULT AddErrorCallback(void * lpData, Callback lpCb);
    HRESULT AddWarningCallback(void * lpData, Callback lpCb);
    HRESULT AddLogCallback(void * lpData, Callback lpCb);

	//HRESULT Release();
};

#endif
