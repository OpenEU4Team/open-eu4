#pragma once

#include "clausewitz/clausewitzlib/commandline.h"
#include "clausewitz/clausewitzlib/dlc.h"
#include "clausewitz/pdx_core/version.h"
#include <vector>

bool BackupLogs(const CString &, const CString &, const std::vector<CString> &, int);                                                                      
void InitializeLogging(const CString &, const CString &, bool, bool, bool, bool);                                                               
void DeInitalizeLogging();                                                                                                       
void WriteGameLogHeader(const CVersion &);                                                                                                 
void PreEnumerateFiles();                                                                                                       
void MountCK2SaveExportFolder();                                                                                                       
void MountGameVFS(CDLCManager &, const CString &, bool);                                                                                           
void InitLocalisation(const SClausewitzCommandLineOptions &, const char *, bool &, bool);                                                                            
void CreateCrashReporter(const CPdxArray<CString> &, bool);                                                                                              
