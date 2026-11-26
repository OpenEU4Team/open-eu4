#pragma once

HELPER const CArray<CBasicChecksum> &CDailyTickCommand::GetChecksums() const
{
    return this->_Checksums;
}
