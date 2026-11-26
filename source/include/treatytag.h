#pragma once

class CScriptablePeaceTreatyDefinition;

enum EStaticTreatyType : int
{
    TREATY_TYPE_GOLD = 0,
    TREATY_TYPE_BECOME_VASSAL = 1,
    TREATY_TYPE_SUBJUGATE_VASSAL = 2,
    TREATY_TYPE_CEDE_PROVINCE = 3,
    TREATY_TYPE_REVOKE_CORE = 4,
    TREATY_TYPE_RETURN_CORE = 5,
    TREATY_TYPE_GIVE_UP_CLAIMS = 6,
    TREATY_TYPE_WAR_REPARATIONS = 7,
    TREATY_TYPE_TRANSFER_TRADE_POWER = 8,
    TREATY_TYPE_HUMILIATE_RIVAL = 9,
    TREATY_TYPE_HUMILIATE = 10,
    TREATY_TYPE_CONCEDE_DEFEAT = 11,
    TREATY_TYPE_REVOKE_REFORM = 12,
    TREATY_TYPE_CHANGE_HRE_RELIGION = 13,
    TREATY_TYPE_ANNUL_TREATIES = 14,
    TREATY_TYPE_RELEASE_NATION = 15,
    TREATY_TYPE_ENFORCE_MILITARY_ACCESS = 16,
    TREATY_TYPE_ENFORCE_FLEET_BASING_RIGHTS = 17,
    TREATY_TYPE_GRANT_INDEPENDENCE = 18,
    TREATY_TYPE_RELEASE_VASSAL = 19,
    TREATY_TYPE_TRANSFER_VASSAL = 20,
    TREATY_TYPE_CHANGE_RELIGION = 21,
    TREATY_TYPE_CHANGE_GOVERNMENT = 22,
    TREATY_TYPE_DISMANTLE_REVOLUTION = 23,
    TREATY_TYPE_REVOKE_ELECTOR = 24,
    TREATY_TYPE_ENFORCE_REBEL_DEMANDS = 25,
    TREATY_TYPE_FORM_PERSONAL_UNION = 26,
    TREATY_TYPE_CONCEDE_COLONIAL_REGION = 27,
    TREATY_TYPE_END_RIVALRY = 28,
    TREATY_TYPE_TAKE_MANDATE = 29,
    TREATY_TYPE_BECOME_TRIBUTARY = 30,
    TREATY_TYPE_SUBJUGATE_TRIBUTARY = 31,
    TREATY_TYPE_STEER_TRADE = 32,
    TREATY_TYPE_JOIN_EMPIRE = 33,
    TREATY_TYPE_SPREAD_REVOLUTION = 34,
    TREATY_TYPE_FORCE_MIGRATION = 35,
    TREATY_TYPE_FORCE_OUT_COLONIZERS = 36,
    TREATY_TYPE_PILLAGE_CAPITAL_STATE = 37,
    TREATY_TYPE_GUARANTEED_JOIN_EMPIRE = 38,
    TREATY_TYPE_SCRIPTABLE = 39,
    NUM_TREATY_TYPES = 40,
};

struct SPeaceTreatyTag
{
public:
    inline explicit SPeaceTreatyTag(EStaticTreatyType StaticType)                                                     
        : _StaticType(StaticType), _pDynamicType(nullptr) {}

    inline EStaticTreatyType GetStaticType() const;                                                                       
    inline const CScriptablePeaceTreatyDefinition *GetDynamicType() const;                                                 

private:
    EStaticTreatyType _StaticType;                                                                
    const CScriptablePeaceTreatyDefinition *_pDynamicType;                                          
};

#include "treatytag.tpp"
