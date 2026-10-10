require("base.nut");
class ZoomVehicles extends ZoomStatic {
    vehicles=[];
    function Save() {
        foreach(v in this.vehicles) if(AIVehicle.IsValidVehicle(v)) AILog.Info("VEHICLE SAVE id="+v+", tile="+AIVehicle.GetLocation(v));
        return { vehicles=this.vehicles };
    }
    function Load(version,data) {
        this.loaded=true;
        if(data!=null && data.rawin("vehicles")) this.vehicles=data.vehicles;
    }
    function Engine(type,kind=0,rail=-1) {
        local list=AIEngineList(type),best=null,price=1000000000;
        for(local e=list.Begin();!list.IsEnd();e=list.Next()) {
            if(!AIEngine.IsBuildable(e)) continue;
            if(type==AIVehicle.VT_RAIL && (AIEngine.IsWagon(e) || !AIEngine.HasPowerOnRail(e,rail) || AIEngine.GetRailType(e)!=rail)) continue;
            if(type==AIVehicle.VT_ROAD) {
                if(!AIEngine.CanRunOnRoad(e,AIRoad.GetCurrentRoadType())) continue;
                local passenger=AICargo.HasCargoClass(AIEngine.GetCargoType(e),AICargo.CC_PASSENGERS);
                if(passenger!=(kind==0)) continue;
            }
            if(type==AIVehicle.VT_AIR && ((AIEngine.GetPlaneType(e)==AIAirport.PT_HELICOPTER)!=(kind==1))) continue;
            if(AIEngine.GetPrice(e)<price) { best=e;price=AIEngine.GetPrice(e); }
        }
        return best;
    }
    function Vehicle(depot,engine,first,second,name) {
        if(engine==null) { AILog.Warning("VEHICLE unavailable "+name);return null; }
        local v=AIVehicle.BuildVehicle(depot,engine);
        if(!AIVehicle.IsValidVehicle(v)) { AILog.Error("VEHICLE build "+name+": "+AIError.GetLastErrorString());return null; }
        AIVehicle.SetName(v,"Zoom "+name);
        if(!AIOrder.AppendOrder(v,first,AIOrder.OF_NONE) || !AIOrder.AppendOrder(v,second,AIOrder.OF_NONE) || !AIVehicle.StartStopVehicle(v)) {
            AILog.Error("VEHICLE orders/start "+name+": "+AIError.GetLastErrorString());return null;
        }
        this.vehicles.append(v);
        AISign.BuildSign(first,"ZOOM "+name);
        AILog.Info("VEHICLE READY "+name+": id="+v+", engine="+engine+", scrollto "+first+", depot="+depot+", second="+second);
        return v;
    }
    function RoadRoute(kind,tram=false) {
        AIRoad.SetCurrentRoadType(AIRoad.ROADTYPE_ROAD);
        if(tram) {
            local types=AIRoadTypeList(AIRoad.ROADTRAMTYPES_TRAM);
            if(types.IsEmpty()) { AILog.Warning("VEHICLE tram unavailable in this base-game world");return; }
            AIRoad.SetCurrentRoadType(types.Begin());
        }
        local engine=this.Engine(AIVehicle.VT_ROAD,kind);
        if(engine==null) { AILog.Warning("VEHICLE road engine unavailable kind="+kind+", tram="+tram);return; }
        local s=this.Rectangle(16,3);
        if(s==null) { AILog.Error("VEHICLE no flat road site");return; }
        local first=this.Tile(s.x,s.y+1),second=this.Tile(s.x+12,s.y+1),depot=this.Tile(s.x+2,s.y+1);
        local stop=kind==0 ? AIRoad.ROADVEHTYPE_BUS : AIRoad.ROADVEHTYPE_TRUCK;
        if(!AIRoad.BuildRoad(this.Tile(s.x,s.y),this.Tile(s.x+12,s.y)) ||
            !AIRoad.BuildRoadStation(first,this.Tile(s.x,s.y),stop,AIStation.STATION_NEW) ||
            !AIRoad.BuildRoadStation(second,this.Tile(s.x+12,s.y),stop,AIStation.STATION_NEW) ||
            !AIRoad.BuildRoadDepot(depot,this.Tile(s.x+2,s.y))) { AILog.Error("VEHICLE road infrastructure: "+AIError.GetLastErrorString());return; }
        AIRoad.BuildRoad(first,this.Tile(s.x,s.y));
        AIRoad.BuildRoad(second,this.Tile(s.x+12,s.y));
        AIRoad.BuildRoad(depot,this.Tile(s.x+2,s.y));
        this.Vehicle(depot,engine,first,second,tram ? "tram" : kind==0 ? "bus" : "truck");
    }
    function TrainRoute(type) {
        AIRail.SetCurrentRailType(type);
        local engine=this.Engine(AIVehicle.VT_RAIL,0,type);
        if(engine==null) { AILog.Warning("VEHICLE train unavailable railtype="+type);return; }
        local s=this.Rectangle(16,2);
        if(s==null) { AILog.Error("VEHICLE no rail site");return; }
        local depot=this.Tile(s.x,s.y),first=this.Tile(s.x+3,s.y),second=this.Tile(s.x+12,s.y);
        if(!AIRail.BuildRailDepot(depot,this.Tile(s.x+1,s.y)) ||
            !AIRail.BuildRailStation(first,AIRail.RAILTRACK_NE_SW,1,2,AIStation.STATION_NEW) ||
            !AIRail.BuildRailStation(second,AIRail.RAILTRACK_NE_SW,1,2,AIStation.STATION_NEW)) { AILog.Error("VEHICLE rail infrastructure: "+AIError.GetLastErrorString());return; }
        for(local dx=1;dx<=11;dx++) if(dx!=3 && dx!=4) AIRail.BuildRailTrack(this.Tile(s.x+dx,s.y),AIRail.RAILTRACK_NE_SW);
        this.Vehicle(depot,engine,first,second,"train railtype "+type);
    }
    function Airport() {
        local type=AIAirport.AT_METROPOLITAN;
        for(local attempt=0;attempt<8;attempt++) {
            local s=this.Rectangle(AIAirport.GetAirportWidth(type),AIAirport.GetAirportHeight(type));
            if(s==null) return null;
            local t=this.Tile(s.x,s.y);
            if(AIAirport.BuildAirport(t,type,AIStation.STATION_NEW)) return t;
            AILog.Warning("VEHICLE airport failed: "+AIError.GetLastErrorString());
            /* Avoid selecting the same unsuitable rectangle on another attempt. */
            AIRoad.BuildRoad(this.Tile(s.x,s.y),this.Tile(s.x+1,s.y));
        }
        return null;
    }
    function AirRoutes() {
        local first=this.Airport(),second=this.Airport();
        if(first==null || second==null) { AILog.Error("VEHICLE airport pair unavailable");return; }
        local depot=AIAirport.GetHangarOfAirport(first);
        this.Vehicle(depot,this.Engine(AIVehicle.VT_AIR,0),first,second,"aircraft");
        this.Vehicle(depot,this.Engine(AIVehicle.VT_AIR,1),first,second,"helicopter");
    }
    function ShipRoute() {
        for(local y=4;y<AIMap.GetMapSizeY()-8;y++) {
            for(local x=4;x<AIMap.GetMapSizeX()-28;x++) {
                local good=true;
                for(local dy=0;dy<4 && good;dy++) for(local dx=0;dx<24;dx++) if(!AITile.IsWaterTile(this.Tile(x+dx,y+dy))) {good=false;break;}
                if(!good) continue;
                local depot=this.Tile(x+1,y+1),first=this.Tile(x+5,y+2),second=this.Tile(x+20,y+2);
                if(!AIMarine.BuildWaterDepot(depot,this.Tile(x+2,y+1))) continue;
                AIMarine.BuildBuoy(first);AIMarine.BuildBuoy(second);
                this.Vehicle(depot,this.Engine(AIVehicle.VT_WATER),first,second,"ship");
                return;
            }
            if(y%8==0) this.Sleep(1);
        }
        AILog.Error("VEHICLE no open water route");
    }
    function Start() {
        this.SetCommandDelay(1);
        if(!this.loaded) {
          AICompany.SetLoanAmount(AICompany.GetMaxLoanAmount());
          try {
            this.RoadRoute(0);this.RoadRoute(1);this.RoadRoute(0,true);
            local types=AIRailTypeList();
            for(local type=types.Begin();!types.IsEnd();type=types.Next()) this.TrainRoute(type);
            this.AirRoutes();this.ShipRoute();
            AILog.Info("VEHICLE FIXTURE COMPLETE: "+this.vehicles.len()+" started vehicles");
          } catch(e) { AILog.Error("VEHICLE exception: "+e); }
        }
        while(true) {
            this.Sleep(10);
            for(local i=0;i<this.vehicles.len();i++) {
                local v=this.vehicles[i];
                if(AIVehicle.IsValidVehicle(v)) AILog.Info("VEHICLE POSITION id="+v+", tile="+AIVehicle.GetLocation(v)+", speed="+AIVehicle.GetCurrentSpeed(v));
            }
        }
    }
}
