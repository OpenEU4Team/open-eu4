#pragma once

HELPER const CString &CDLCDescriptor::GetFileName() const
{
    return this->_FileName;
}

HELPER bool CDLCDescriptor::IsDisabledNotLoggedIn() const
{
    return this->_bDisabledNotLoggedIn;
}

HELPER void CDLCManager::SInitSettings::SetFolders(const CString &dlc, const CString &builtin, const CString &mod)
{
    this->_DLCFolder = dlc;
    this->_BuiltInDLCFolder = builtin;
    this->_ModFolder = mod;
}

HELPER CPdxArray<CString> &CDLCManager::SInitSettings::AccessBuiltInDLCNames()
{
    return this->_BuiltInDLCNames;
}

inline const CPdxArray<CString> &CDLCManager::GetEnabledMods() const
{
    return this->_EnabledMods;
}
