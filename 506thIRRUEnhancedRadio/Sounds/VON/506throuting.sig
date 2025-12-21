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
   tl 90 -114
   children {
    4
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
    2
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
    5
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
 }
 Outputs {
  IOPItemOutputClass {
   id 2
   name "CenterAmp"
   tl 422 0
   input 6
  }
  IOPItemOutputClass {
   id 4
   name "LeftAmp"
   tl 426 -89
   input 3
  }
  IOPItemOutputClass {
   id 5
   name "RightAmp"
   tl 425 88
   input 7
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