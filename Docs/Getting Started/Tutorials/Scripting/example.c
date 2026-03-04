// PocketMage scripting example
// Based on Wrench by jingoro2112 (GH)
// Edit with potion: pot example.c
// Compile and run: brew example.c

// Declare a variable
var x = ""; // "" indicates str

// Prompt user to enter number
// Set x to entered number
x = prompt("Enter a number");

// Display x on OLED
oledWord(x);

// Print x to terminal
print(x);

// Update the terminal
// (refresh the OLED)
updateTerm();

// Initialize an iteration var i
var i = 0;

// Loop while i < x
// Us toInt to convert str to int
while (i < toInt(x)) {
  // Iterate
  i = i + 1;

  // Display i on OLED
  oledWord(i);

  // Wait 500ms
  delay(500);
}

// Print the final i to the term
print(i);

// Generate a random number
var randomInt = random(0, 100);
print("Random Number: " + randomInt);

// Update the terminal
// (refresh the OLED)
updateTerm();