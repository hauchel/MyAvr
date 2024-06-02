#ifndef TETRIS
//calling: inp contains function requested, stack params
void doFuncs(byte was) {
  inp = inpPop();
  switch (was) {
    case 1:   // inp to cycle # leds up
      inp += 1;
      if (inp >= LEDNUM) {
        inp = 0;
      }
      break;
    case 2:   // inp to cycle # leds down
      inp -= 1;
      if (inp < 0) {
        inp = LEDNUM - 1;
      }
      break;
    case 3:   // inp to cycle # leds right
      inp -= 1;
      if (inp < 0) {
        inp = LEDNUM - 1;
      }
      break;
    case 4:   // inp to cycle # leds left
      inp -= 1;
      if (inp < 0) {
        inp = LEDNUM - 1;
      }
      break;
    default:
      errF(F("doFuncs"), inp);
  } //switch
}
#endif

void doRC5() {
  msgF(F("RC5 "), rc5_command);
  if (rc5_command <= 9) {
    doCmd(rc5_command + 48);
  } else {
    switch (rc5_command) {
      case 11:   // Store
        break;
      case 13:   // Mute
        break;
      case 43:   // >
        break;
      case 25:   // R
        doCmd('r');
        break;
      case 23:   // G
        doCmd('b');
        break;
      case 27:   // Y
        doCmd('g');
        break;
      case 36:   // B
        doCmd('b');
        break;
      case 44:   // Punkt
        doCmd('z');
        break;
      case 57:   // Ch+
        break;
      case 56:   // Ch+
        break;
      default:
        msgF(F("Which RC5?"), rc5_command);
    } // case
  } // else
}
