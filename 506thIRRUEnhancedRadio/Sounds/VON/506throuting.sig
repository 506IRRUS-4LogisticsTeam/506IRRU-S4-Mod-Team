AudioSignalResClass {
 Inputs {
  IOPItemInputClass {
   id 1
   name "EarRouting"
   tl -253.584 -46.442
   children {
    20 21
   }
  }
  IOPInputVariableClass {
   id 25
   name "Channel Volume"
   tl 84.429 275.99
   children {
    23 24
   }
   varName "ChannelVolume"
   varResource "{23599C437CC8463D}Sounds/VON/RadioEarRouting.conf"
  }
 }
 Ops {
  IOPItemOpConvertorClass {
   id 20
   name "LeftChannelVol"
   tl 74.821 -185.388
   children {
    23
   }
   inputs {
    ConnectionClass "1:0" {
     id 1
     port 0
    }
   }
   Default 6
   Intervals {
    IOPItemOpConvertorRange LeftChannel {
     min 1
     max 2
    }
   }
  }
  IOPItemOpConvertorClass {
   id 21
   name "RightChannelVol"
   tl 76.623 124.05
   children {
    24
   }
   inputs {
    ConnectionClass "1:0" {
     id 1
     port 0
    }
   }
   Default 6
   Intervals {
    IOPItemOpConvertorRange RightChannel {
     min 2
     max 3
    }
   }
  }
  IOPItemOpMulClass {
   id 23
   name "Left Mul"
   tl 476.603 -180.532
   children {
    22
   }
   inputs {
    ConnectionClass "25:0" {
     id 25
     port 0
    }
    ConnectionClass "20:0" {
     id 20
     port 0
    }
   }
  }
  IOPItemOpMulClass {
   id 24
   name "Right Mul"
   tl 465.299 120.338
   children {
    2
   }
   inputs {
    ConnectionClass "25:0" {
     id 25
     port 0
    }
    ConnectionClass "21:0" {
     id 21
     port 0
    }
   }
  }
 }
 Outputs {
  IOPItemOutputClass {
   id 2
   name "RightVol"
   tl 888.513 125.041
   input 24
  }
  IOPItemOutputClass {
   id 22
   name "LeftVol"
   tl 888.012 -183.347
   input 23
  }
 }
 Input_Order {
  ItemDetailListItemClass EarRouting {
   Name "EarRouting"
   Id 1
  }
 }
 Output_Order {
  ItemDetailListItemClass RightVol {
   Name "RightVol"
   Id 2
  }
  ItemDetailListItemClass LeftVol {
   Name "LeftVol"
   Id 22
  }
 }
}