AudioSignalResClass {
 Inputs {
  IOPItemInputClass {
   id 1
   name "EarRouting"
   tl -274 8
   children {
    3 6 7
   }
  }
 }
 Ops {
  IOPItemOpConditionClass {
   id 3
   name "ConditionLeft"
   tl 79.565 -135.9
   children {
    14
   }
   inputs {
    ConnectionClass "1:0" {
     id 1
     port 0
    }
   }
   "Condition Type" 250
   Comparator 2
  }
  IOPItemOpConditionClass {
   id 6
   name "ConditionCenter"
   tl 89.135 -15.894
   children {
    15
   }
   inputs {
    ConnectionClass "1:0" {
     id 1
     port 0
    }
   }
   "Condition Type" 250
  }
  IOPItemOpConditionClass {
   id 7
   name "ConditionRight"
   tl 89.135 84.106
   children {
    16
   }
   inputs {
    ConnectionClass "1:0" {
     id 1
     port 0
    }
   }
   "Condition Type" 238
   Comparator 1
  }
  IOPItemOpInterpolateClass {
   id 14
   name "Signal interpolation :D"
   tl 396.384 -337.865
   children {
    4
   }
   inputs {
    ConnectionClass "3:0" {
     id 3
     port 0
    }
   }
   "Y min" -60
   "Y max" 5
  }
  IOPItemOpInterpolateClass {
   id 15
   name "Signal interpolation :D"
   tl 392.93 -155.514
   children {
    2
   }
   inputs {
    ConnectionClass "6:0" {
     id 6
     port 0
    }
   }
   "Y min" -60
   "Y max" 5
  }
  IOPItemOpInterpolateClass {
   id 16
   name "Signal interpolation :D"
   tl 403.019 38.293
   children {
    5
   }
   inputs {
    ConnectionClass "7:0" {
     id 7
     port 0
    }
   }
   "Y min" -60
   "Y max" 5
  }
 }
 Outputs {
  IOPItemOutputClass {
   id 2
   name "CenterAmp"
   tl 817.6 -10.35
   input 15
  }
  IOPItemOutputClass {
   id 4
   name "LeftAmp"
   tl 821.6 -99.35
   input 14
  }
  IOPItemOutputClass {
   id 5
   name "RightAmp"
   tl 820.6 77.65
   input 16
  }
 }
 Input_Order {
  ItemDetailListItemClass EarRouting {
   Name "EarRouting"
   Id 1
  }
 }
 Output_Order {
  ItemDetailListItemClass CenterAmp {
   Name "CenterAmp"
   Id 2
  }
  ItemDetailListItemClass LeftAmp {
   Name "LeftAmp"
   Id 4
  }
  ItemDetailListItemClass RightAmp {
   Name "RightAmp"
   Id 5
  }
 }
}