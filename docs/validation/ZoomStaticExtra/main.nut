require("base.nut");
class ZoomStaticExtra extends ZoomStatic {
    function Start() {
        this.SetCommandDelay(1);
        if(this.loaded) { while(true) this.Sleep(1000); }
        AICompany.SetLoanAmount(AICompany.GetMaxLoanAmount());
        AIRoad.SetCurrentRoadType(AIRoad.ROADTYPE_ROAD);
        AIRail.SetCurrentRailType(0);
        local s=this.Rectangle(10,10);
        if(s!=null) {
            local road=this.Tile(s.x+3,s.y+3);
            this.Check(AITile.RaiseTile(road,AITile.SLOPE_N),"raised road slope",road);
            this.Check(AIRoad.BuildRoad(this.Tile(s.x+1,s.y+3),this.Tile(s.x+5,s.y+3)),"road foundation",road);
            local rail=this.Tile(s.x+3,s.y+7);
            this.Check(AITile.RaiseTile(rail,AITile.SLOPE_N),"raised rail slope",rail);
            this.Check(AIRail.BuildRailTrack(rail,AIRail.RAILTRACK_NE_SW),"rail foundation",rail);
            local hill=this.Tile(s.x+7,s.y+5);
            this.Check(AITile.RaiseTile(hill,AITile.SLOPE_N),"hill",hill);
            this.Check(AITile.RaiseTile(hill,AITile.SLOPE_N),"steep hill",hill);
            AILog.Info("STATIC slope after raising twice="+AITile.GetSlope(hill));
        }
        local trams=AIRoadTypeList(AIRoad.ROADTRAMTYPES_TRAM);
        if(trams.IsEmpty()) AILog.Info("STATIC tram unavailable: no NewGRF tram road type in this base-game world");
        else {
            AIRoad.SetCurrentRoadType(trams.Begin());
            local t=this.Rectangle(10,3);
            if(t!=null) {
                this.Check(AIRoad.BuildRoad(this.Tile(t.x,t.y),this.Tile(t.x+8,t.y)),"tram track",this.Tile(t.x+4,t.y));
                this.Check(AIRoad.BuildRoadDepot(this.Tile(t.x+2,t.y+1),this.Tile(t.x+2,t.y)),"tram depot",this.Tile(t.x+2,t.y+1));
            }
        }
        AILog.Info("STATIC EXTRAS COMPLETE: no vehicles constructed");
        while(true) this.Sleep(1000);
    }
}
