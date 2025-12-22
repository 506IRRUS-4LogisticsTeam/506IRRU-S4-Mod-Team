AudioSignalResClass {
 Inputs {
  IOPItemInputClass {
   id 1
   name "EarRouting"
   tl -274 8
   children {
    20
   }
  }
 }
 Ops {
  IOPItemOpConvertorClass {
   id 20
   name "Center"
   tl 109.679 -4.53
   children {
    2
   }
   inputs {
    ConnectionClass "1:0" {
     id 1
     port 0
    }
   }
   Default 0
   DefaultFromInput 1
   Intervals {
    IOPItemOpConvertorRange CenterRouting {
    }
    IOPItemOpConvertorRange RightRouting {
     min 1
     max 2
     "out" -1.57
    }
    IOPItemOpConvertorRange LeftRouting {
     min 2
     max 3
     "out" 1.57
    }
   }
  }
 }
 Outputs {
  IOPItemOutputClass {
   id 2
   name "PanAngle"
   tl 454.6 -12.35
   input 20
  }
 }
 Input_Order {
  ItemDetailListItemClass EarRouting {
   Name "EarRouting"
   Id 1
  }
 }
 Output_Order {
  ItemDetailListItemClass PanAngle {
   Name "PanAngle"
   Id 2
  }
 }
}