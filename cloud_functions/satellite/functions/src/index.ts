import * as functions from "firebase-functions";
import admin = require("firebase-admin");
// // Start writing Firebase Functions
// // https://firebase.google.com/docs/functions/typescript

//  const admin =
admin.initializeApp();
const db = admin.firestore();

/**
 *
 * @param {stirng} str1 The string
 * @return {string} The result
 */
function hexToAscii(str1 : string) {
  const hex = str1.toString();
  let str= "";
  for (let n= 0; n < hex.length; n += 2) {
    str += String.fromCharCode(parseInt(hex.substr(n, 2), 16));
  }
  return str;
}

export const helloWorld = functions.https.onRequest((request, response) => {
  functions.logger.info("Got a Message from RockBloick !", request);

  const requestJson = request.body;

  functions.logger.info("Request Json is ", requestJson);

  const rawData = requestJson["data"] ||
  "4572726f72206661696c656420746f206765742064617461";
  functions.logger.info("Data field is : ", rawData);

  const normalData = hexToAscii(rawData);

  functions.logger.info("Decoded raw data is : ", normalData);

  const jsonObj :{[name:string] : string}= {};
  jsonObj["normalData"] = normalData;

  db.collection("test").doc(requestJson["transmit_time"]).set(jsonObj);

  response.send("Hi");
});
