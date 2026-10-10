class ZoomStaticInfo extends AIInfo {
    function GetAuthor()      { return "OpenTTD smooth zoom validation"; }
    function GetName()        { return "ZoomStatic"; }
    function GetShortName()   { return "ZSTA"; }
    function GetDescription() { return "Builds static infrastructure without vehicles for fractional zoom validation."; }
    function GetVersion()     { return 1; }
    function GetAPIVersion()  { return "16"; }
    function GetDate()        { return "2026-10-08"; }
    function CreateInstance() { return "ZoomStatic"; }
    function UseAsRandomAI()  { return false; }
}
RegisterAI(ZoomStaticInfo());
