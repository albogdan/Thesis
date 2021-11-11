
long rand_num_1;
long rand_num_2;

void setup() {
  Serial.begin(115200);
  // put your setup code here, to run once:
  randomSeed(analogRead(0));
}

void loop() {
  // Generate a random number from 0-100
  rand_num_1 = random(100);

  // Generate a random number from 0-10000
  rand_num_2 = random(10000);

  Serial.print(rand_num_1);
  Serial.print(",");
  Serial.println(rand_num_2);
  
  // Run this code only every 2 seconds
  delay(2000);
}
