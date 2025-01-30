//calling: inp contains function requested, stack params
void doFuncs(byte was) {
  inp = inpPop();
  switch (was) {
    case 1:  // inp to cycle # leds up
      inp += 1;
      if (inp >= LEDNUM) {
        inp = 0;
      }
      break;
    case 2:  // inp to cycle # leds down
      if (inp == 0) {
        inp = LEDNUM - 1;
      } else {
        inp -= 1;
      }
      break;
    case 3:  // inp to cycle # leds right
      if (inp == 0) {
        inp = LEDNUM - 1;
      } else {
        inp -= 1;
      }
      break;
    case 4:  // inp to cycle # leds left
      if (inp == 0) {
        inp = LEDNUM - 1;
      } else {
        inp -= 1;
      }
      break;
    default:
      errF(F("doFuncs"), inp);
  }  //switch
}

