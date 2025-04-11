Within Reforger Tools > TRF > Sounds > VON > VON_DIRECTION.conf 

Edit the "VON_LEFT/RIGHT" variables to Source = "Constant" and a "Value" of 0 to 1 to reduce or increase the volume in the respective side. This works and has been confirmed as working in dedicated multiplayer server. However, I need to develop the script so I can set these variables to "External" and determine the behaviour of the variable based on some other function. Current attempt was using a script to find the "Radio Type" i.e ANPRC77 to set the value for variable VON_LEFT to 0.0 and VON_RIGHT to 3.0 so all audio would play through the right channel.


// === Folder Structure ===
//
// - Scripts
//   - Game
//     - ToastyVONRouting
//       - VONRoutingComponent.c
//       - SCR_PlayerController.c <-- Not compiling as of yet. 
// - Configs
//   - keyBindingMenu.conf????? <-- Need to check this
// - Sounds
//   - VON
//     - VON_DIRECTION.conf 