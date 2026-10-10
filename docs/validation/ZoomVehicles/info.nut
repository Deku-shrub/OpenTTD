class ZoomVehiclesInfo extends AIInfo {
    function GetAuthor()      { return "OpenTTD smooth zoom validation"; }
    function GetName()        { return "ZoomVehicles"; }
    function GetShortName()   { return "ZVEH"; }
    function GetDescription() { return "Builds moving vehicle routes for fractional viewport validation."; }
    function GetVersion()     { return 1; }
    function GetAPIVersion()  { return "16"; }
    function GetDate()        { return "2026-10-08"; }
    function CreateInstance() { return "ZoomVehicles"; }
    function UseAsRandomAI()  { return false; }
}
RegisterAI(ZoomVehiclesInfo());
