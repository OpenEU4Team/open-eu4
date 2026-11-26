#pragma once
#include "text.h"
bool VFSInit(const char *pRoot, const char *pUser, bool bWrite);                                  

CString VFSGetCustomUserDir(const char *path);                                            
CString VFSGetDefaultUserDir(const char *name);                                            
void VFSMountCustomUserDir(const char *path);                                               
