#ifndef BUTTONS_H
#define BUTTONS_H

// Button names
#define BUTTON_A      0 // Start/stop logging
#define BUTTON_B      1 // Set logging interval


struct button {
  uint8_t pin;
  uint8_t inverted;
};


class Buttons {
  private:
    int pressedButton;  // Stores the button that was pressed
    int lastPressed;
    int debounceCount;  // Number of times we've seen the same input
    #define DEBOUNCE_INTERVAL 500  // Number of times we need s
    
  public:
    // Initialize the buttons class
    void setup();
    
    // Scan for button presses
    void buttonTask();
    
    // Returns true if a button is pressed
    bool isPressed();
    
    // If a button is pressed, get it!
    int getPressed();
};


#endif // BUTTONARRAY_HH