require("vehicles.nut");
class ZoomCoverage extends ZoomVehicles {
    function Start() {
        this.SetCommandDelay(1);
        if(!this.loaded) {
            AICompany.SetLoanAmount(AICompany.GetMaxLoanAmount());
            AICompany.SetPrimaryLiveryColour(AICompany.LS_DEFAULT, AICompany.COLOUR_PURPLE);
            AICompany.SetSecondaryLiveryColour(AICompany.LS_DEFAULT, AICompany.COLOUR_ORANGE);
            try {
                this.RoadRoute(0, true);
                this.RoadRoute(1);
                local types=AIRailTypeList();
                for(local type=types.Begin(); !types.IsEnd(); type=types.Next()) this.TrainRoute(type);
                this.Rail();
                local signs=["Zoom scaling", "مقياس التكبير", "拡大縮小の表示", "Échelle du zoom"];
                foreach(i,text in signs) {
                    local tile=this.Tile(80,80+i*3);
                    local sign=AISign.BuildSign(tile,text);
                    AILog.Info("COVERAGE LABEL id="+sign+", scrollto "+tile+", text="+text);
                }
                local industries=AIIndustryList();
                for(local i=industries.Begin(); !industries.IsEnd(); i=industries.Next()) {
                    AILog.Info("COVERAGE INDUSTRY id="+i+", type="+AIIndustry.GetIndustryType(i)+", scrollto "+AIIndustry.GetLocation(i));
                }
                AILog.Info("COVERAGE COMPLETE vehicles="+this.vehicles.len());
            } catch(e) { AILog.Error("COVERAGE exception: "+e); }
        }
        while(true) {
            this.Sleep(30);
            foreach(v in this.vehicles) if(AIVehicle.IsValidVehicle(v)) {
                AILog.Info("COVERAGE VEHICLE id="+v+", tile="+AIVehicle.GetLocation(v)+", speed="+AIVehicle.GetCurrentSpeed(v));
            }
        }
    }
}
