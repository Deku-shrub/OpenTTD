class ZoomCoverageInfo extends AIInfo {
    function GetAuthor() { return "OpenTTD smooth zoom validation"; }
    function GetName() { return "ZoomCoverage"; }
    function GetShortName() { return "ZCOV"; }
    function GetDescription() { return "Tram, waypoint, Unicode label and dynamic-world zoom coverage."; }
    function GetVersion() { return 1; }
    function GetAPIVersion() { return "16"; }
    function GetDate() { return "2026-10-10"; }
    function CreateInstance() { return "ZoomCoverage"; }
    function UseAsRandomAI() { return false; }
}
RegisterAI(ZoomCoverageInfo());
