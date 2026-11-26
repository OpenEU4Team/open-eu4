#pragma once

enum CombatType : int
{
    _NO_COMBAT_ = 0,
    _COMBAT_LAND_ = 1,
    _COMBAT_NAVAL_ = 2,
    _COMBAT_SIEGE_ = 3,
};

enum EUnitMission : int
{
    NM_NONE = 0,
    NM_OTHER = 0,
    NM_PROTECT_TRADE = 1,
    NM_PRIVATEER = 2,
    NM_HUNT_PIRATES = 3,
    NM_PATROL = 4,
    NM_EXPLORE = 5,
    NM_EVADE = 6,
    NM_HUNT = 7,
    NM_BLOCKADE = 8,
    NM_INTERCEPT = 9,
    NM_REPAIR = 10,
    NM_EXPLORE_CIRCUMNAVIGATE = 11,
    AM_EXPLORE_LAND = 12,
    AM_HUNT_REBELS = 13,
    AM_CARPET_SIEGE = 14,
    AM_DRILL_ARMY = 15,
    NM_NUM_OF = 16,
};

enum WAR_GOAL_TYPES : int
{
    TAKE_COLONY = 0,
    TAKE_CORE = 1,
    TAKE_BORDER = 2,
    TAKE_PROVINCE = 3,
    TAKE_CAPITAL = 4,
    DEFEND_CAPITAL = 5,
    DEFEND_COUNTRY = 6,
    NAVAL_SUPERIORITY = 7,
    SUPERIORITY = 8,
    TAKE_REGION = 9,
    BLOCKADE_PORTS = 10,
    NUM_WAR_GOAL_TYPES = 11,
    INVALID_WAR_GOAL = 12,
};

enum AIBudgetType : int
{
    ADVISORS = 0,
    SAVINGS = 1,
    COLONISTS = 2,
    MISSIONARIES = 3,
    ARMIES = 4,
    NAVIES = 5,
    SUBSIDIES = 6,
    FORTS = 7,
    CORRUPTION = 8,
    STATES = 9,
    MAX = 10,
};

enum EFocalPointState : signed char
{
    Inactive = 0,
    GatheringOffensive = 1,
    GatheringDefensive = 2,
    LaunchedOffensive = 3,
    LaunchedDefensive = 4,
};

enum class SubUnitType : int
{
    FIRST_LAND = 0,
    INFANTRY = 0,
    CAVALRY = 1,
    ARTILLERY = 2,
    MAX_LAND = 3,
    HEAVY_SHIP = 3,
    LIGHT_SHIP = 4,
    GALLEY = 5,
    TRANSPORT = 6,
    MAX = 7,
    NO_SUB_UNIT_TYPE = -1,
};

enum CombatPhase : int
{
    _FIRE_PHASE_ = 0,
    _SHOCK_PHASE_ = 1,
};

enum class ESeaType : char
{
    OPEN_SEA = 0,
    COASTAL = 1,
    INLAND = 2,
    MAX = 3,
};

enum class EStopMissionReason : unsigned char
{
    CarryOn = 0,
    Generic = 1,
    Exiled = 2,
    NoLeader = 3,
    Retreating = 4,
    Completed = 5,
    CannotReach = 6,
    NotInColonialRegion = 7,
    NowhereLeftToExplore = 8,
    Mothballed = 9,
    OutOfTradeRange = 10,
    StartingNewMission = 11,
    NoLightShips = 12,
    Exploring = 13,
    MercenarysCantDrill = 14,
    CondottieriCantDrill = 15,
    NotOwner = 16,
    AttachedToOtherUnit = 17,
    NotControllerOfProvince = 18,
    InCombat = 19,
    Moving = 20,
    CantMove = 21,
    NoTradeNode = 22,
    NoInterestInTradeNode = 23,
    OverriddenByNewCommand = 24,
    TooManyShips = 25,
    NeededForWar = 26,
    TooManyPrivateers = 27,
    NeededForTransportation = 28,
    TooManyUnitsForExploration = 29,
    NotEnoughUnitsForExploration = 30,
    Drilling = 31,
    PreparingForWar = 32,
    HighAttrition = 33,
    CantAffordIt = 34,
    HostilesClose = 35,
    RebelThreat = 36,
    NeededForExploration = 37,
    LeaderCannotExplore = 38,
    Arrived = 39,
    CannotBlockade = 40,
    Merging = 41,
    WaitingForReinforcements = 42,
    LowMorale = 43,
    NoTransportCapacity = 44,
    NotAtWar = 45,
    LowStrength = 46,
    FleetNotOwned = 47,
    NeedToUnload = 48,
    FleetNeedsToMove = 49,
    NeedToLoad = 50,
    Splinter = 51,
    NeededForHomeFleet = 52,
    PerformingAction = 53,
    UserAction = 54,
    TechDoesntAllowIt = 55,
};

enum class ESpecialUnitCategory : int
{
    NO_REGIMENT = -1,
    NORMAL = 0,
    MERCENARY = 1,
    BANNER = 2,
    STRELTSY = 3,
    COSSACKS = 4,
    JANISSARIES = 5,
    RAJPUT = 6,
    MARINES = 7,
    REVOLUTIONARY_GUARD = 8,
    CAWA = 9,
    CAROLEAN = 10,
    HUSSARS = 11,
    TERCIO = 12,
    MUSKETEER = 13,
    SAMURAI = 14,
    GEOBUKSEON = 15,
    MAN_OF_WAR = 16,
    GALLEON = 17,
    GALLEASS = 18,
    CARAVEL = 19,
    VOC_INDIAMEN = 20,
    QIZILBASH = 21,
    MAMLUKS = 22,
    MAX = 23,
    NON_MERCENARY = 23,
};

enum AIMoveResult : int
{
    _AI_MOVE_RESULT_OK_ = 0,
    _AI_MOVE_RESULT_NOTNEEDED_ = 1,
    _AI_MOVE_RESULT_NOPATH_ = 2,
    _AI_MOVE_RESULT_CANTMOVE_ = 3,
    _AI_MOVE_RESULT_UNSAFE_ = 4,
};

enum ESubjectMilitaryFocus : int
{
    _FOCUS_NONE_ = 0,
    _FOCUS_AGGRESSIVE_ = 1,
    _FOCUS_SUPPORTIVE_ = 2,
    _FOCUS_PASSIVE_ = 3,
    _FOCUS_DEFENSIVE_ = 4,
    _FOCUS_SIEGE_ = 5,
    _NUM_OF_FOCUSES_ = 6,
};

enum EConstructionType : int
{
    MERCHANT = 0,
    DIPLOMAT = 1,
    ARMY = 2,
    NAVY = 3,
    BUILDING = 4,
    MISSIONARY = 5,
    CORE = 6,
    CULTURE = 7,
    GREAT_PROJECT = 8,
    SIBERIAN = 9,
    COLONIST = 10,
    CENTRALIZE_STATE = 11,
    ANY = 12,
};

enum ControllerType : int
{
    _CONTROLLER_AI_ = 0,
    _CONTROLLER_LOCAL_PLAYER_ = 1,
    _CONTROLLER_OTHER_PLAYER_ = 2,
};
