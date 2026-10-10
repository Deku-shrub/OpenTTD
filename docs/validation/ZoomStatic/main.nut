class ZoomStatic extends AIController {
    loaded = false;
    function Save() { return { complete=true }; }
    function Load(version,data) { this.loaded=true; }
    function Tile(x,y) { return AIMap.GetTileIndex(x,y); }
    function Check(ok, name, tile) {
        if (ok) {
            AISign.BuildSign(tile, "ZOOM " + name);
            AILog.Info("STATIC READY " + name + ": scrollto " + tile);
        } else AILog.Warning("STATIC FAILED " + name + ": " + AIError.GetLastErrorString());
        return ok;
    }
    function Rectangle(w,h) {
        for(local y=12;y<AIMap.GetMapSizeY()-h-12;y++) {
            for(local x=12;x<AIMap.GetMapSizeX()-w-12;x++) {
                local good=true;
                local level=AITile.GetMinHeight(this.Tile(x,y));
                for(local dy=0;dy<h && good;dy++) for(local dx=0;dx<w;dx++) {
                    local t=this.Tile(x+dx,y+dy);
                    if(!AITile.IsBuildable(t) || AITile.GetSlope(t)!=AITile.SLOPE_FLAT || AITile.GetMinHeight(t)!=level) { good=false;break; }
                }
                if(good) return {x=x,y=y};
            }
            if(y%8==0) this.Sleep(1);
        }
        return null;
    }
    function Rail() {
        local types=AIRailTypeList();
        if(types.IsEmpty()) return;
        AIRail.SetCurrentRailType(types.Begin());
        local s=this.Rectangle(14,5);
        if(s==null) { AILog.Warning("STATIC no rail rectangle"); return; }
        for(local dx=0;dx<12;dx++) this.Check(AIRail.BuildRailTrack(this.Tile(s.x+dx,s.y),AIRail.RAILTRACK_NE_SW),"rail",this.Tile(s.x+dx,s.y));
        this.Check(AIRail.BuildRailStation(this.Tile(s.x+1,s.y+2),AIRail.RAILTRACK_NE_SW,2,4,AIStation.STATION_NEW),"rail station",this.Tile(s.x+1,s.y+2));
        this.Check(AIRail.BuildRailDepot(this.Tile(s.x+10,s.y+2),this.Tile(s.x+11,s.y+2)),"rail depot",this.Tile(s.x+10,s.y+2));
        this.Check(AIRail.BuildSignal(this.Tile(s.x+6,s.y),this.Tile(s.x+7,s.y),AIRail.SIGNALTYPE_PBS),"path signal",this.Tile(s.x+6,s.y));
        this.Check(AIRail.BuildRailWaypoint(this.Tile(s.x+8,s.y)),"waypoint",this.Tile(s.x+8,s.y));
        local bridges=AIBridgeList_Length(4);
        for(local b=bridges.Begin();!bridges.IsEnd();b=bridges.Next()) {
            local t=this.Tile(s.x+7,s.y+4);
            local end=this.Tile(s.x+10,s.y+4);
            local good=false;
            { local test=AITestMode();good=AIBridge.BuildBridge(AIVehicle.VT_RAIL,b,t,end); }
            if(good) { this.Check(AIBridge.BuildBridge(AIVehicle.VT_RAIL,b,t,end),"rail bridge",t);break; }
        }
        for(local type=types.Begin();!types.IsEnd();type=types.Next()) {
            AIRail.SetCurrentRailType(type);
            local row=this.Rectangle(8,3);
            if(row==null) continue;
            for(local dx=0;dx<8;dx++) AIRail.BuildRailTrack(this.Tile(row.x+dx,row.y),AIRail.RAILTRACK_NE_SW);
            this.Check(AIRail.BuildRailStation(this.Tile(row.x,row.y+1),AIRail.RAILTRACK_NE_SW,1,4,AIStation.STATION_NEW),"rail type "+type,this.Tile(row.x,row.y+1));
        }
        AIRail.SetCurrentRailType(types.Begin());
    }
    function RoadAirport() {
        AIRoad.SetCurrentRoadType(AIRoad.ROADTYPE_ROAD);
        local s=this.Rectangle(14,8);
        if(s==null) return;
        local t=this.Tile(s.x,s.y);
        this.Check(AIRoad.BuildRoad(t,this.Tile(s.x+12,s.y)),"road",t);
        this.Check(AIRoad.BuildRoadDepot(this.Tile(s.x+2,s.y+1),this.Tile(s.x+2,s.y)),"road depot",this.Tile(s.x+2,s.y+1));
        this.Check(AIRoad.BuildRoadStation(this.Tile(s.x+5,s.y+1),this.Tile(s.x+5,s.y),AIRoad.ROADVEHTYPE_BUS,AIStation.STATION_NEW),"bus stop",this.Tile(s.x+5,s.y+1));
        local airport_type=AIAirport.IsValidAirportType(AIAirport.AT_SMALL) ? AIAirport.AT_SMALL : AIAirport.AT_COMMUTER;
        local airport_site=this.Rectangle(AIAirport.GetAirportWidth(airport_type),AIAirport.GetAirportHeight(airport_type));
        if(airport_site!=null) this.Check(AIAirport.BuildAirport(this.Tile(airport_site.x,airport_site.y),airport_type,AIStation.STATION_NEW),"airport",this.Tile(airport_site.x,airport_site.y));
        local bridges=AIBridgeList_Length(4);
        for(local b=bridges.Begin();!bridges.IsEnd();b=bridges.Next()) {
            local start=this.Tile(s.x+1,s.y+5),end=this.Tile(s.x+4,s.y+5),good=false;
            { local test=AITestMode();good=AIBridge.BuildBridge(AIVehicle.VT_ROAD,b,start,end); }
            if(good) { this.Check(AIBridge.BuildBridge(AIVehicle.VT_ROAD,b,start,end),"road bridge",start);break; }
        }
        local canals=this.Rectangle(5,2);
        if(canals!=null) for(local dx=0;dx<5;dx++) this.Check(AIMarine.BuildCanal(this.Tile(canals.x+dx,canals.y)),"canal",this.Tile(canals.x+dx,canals.y));
    }
    function Landscape() {
        local tunnel=false,road_tunnel=false,dock=false,depot=false,lock=false;
        for(local y=4;y<AIMap.GetMapSizeY()-4;y++) {
            for(local x=4;x<AIMap.GetMapSizeX()-4;x++) {
                local t=this.Tile(x,y);
                if(!tunnel && AITile.GetSlope(t)!=AITile.SLOPE_FLAT) {
                    local good=false;
                    { local test=AITestMode();good=AITunnel.BuildTunnel(AIVehicle.VT_RAIL,t); }
                    if(good) tunnel=this.Check(AITunnel.BuildTunnel(AIVehicle.VT_RAIL,t),"rail tunnel",t);
                }
                if(!dock && AITile.IsCoastTile(t)) {
                    local good=false;
                    { local test=AITestMode();good=AIMarine.BuildDock(t,AIStation.STATION_NEW); }
                    if(good) dock=this.Check(AIMarine.BuildDock(t,AIStation.STATION_NEW),"dock",t);
                }
                if(!road_tunnel && AITile.GetSlope(t)!=AITile.SLOPE_FLAT) {
                    local good=false;
                    { local test=AITestMode();good=AITunnel.BuildTunnel(AIVehicle.VT_ROAD,t); }
                    if(good) road_tunnel=this.Check(AITunnel.BuildTunnel(AIVehicle.VT_ROAD,t),"road tunnel",t);
                }
                if(!lock && AITile.GetSlope(t)!=AITile.SLOPE_FLAT) {
                    local good=false;
                    { local test=AITestMode();good=AIMarine.BuildLock(t); }
                    if(good) lock=this.Check(AIMarine.BuildLock(t),"lock",t);
                }
                if(!depot && AITile.IsWaterTile(t) && AITile.IsWaterTile(this.Tile(x+1,y))) {
                    local good=false;
                    { local test=AITestMode();good=AIMarine.BuildWaterDepot(t,this.Tile(x+1,y)); }
                    if(good) depot=this.Check(AIMarine.BuildWaterDepot(t,this.Tile(x+1,y)),"ship depot",t);
                }
                if(tunnel && road_tunnel && dock && depot && lock) return;
            }
            if(y%8==0) this.Sleep(1);
        }
        AILog.Info("STATIC landscape results tunnel="+tunnel+", dock="+dock+", depot="+depot);
    }
    function Start() {
        this.SetCommandDelay(1);
        if(this.loaded) { while(true) this.Sleep(1000); }
        AICompany.SetLoanAmount(AICompany.GetMaxLoanAmount());
        try { this.Rail(); this.RoadAirport(); this.Landscape(); AILog.Info("STATIC FIXTURE COMPLETE: no vehicles constructed"); }
        catch(e) { AILog.Error("STATIC exception: "+e); }
        while(true) this.Sleep(1000);
    }
}
