

#include "NimBLEDevice.h"
#include <sstream>
#include <iomanip>

bool g_do_nimble_connect = false;
unsigned long g_last_numble_scan = 0;
static NimBLEAdvertisedDevice* g_remote_device = nullptr;

class MyAdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
      Serial.printf("BLE: Advertised Device: %s\n", advertisedDevice->toString().c_str());
      int count = advertisedDevice->getServiceUUIDCount();
      for(int i = 0; i < count; ++i)
          Serial.printf("BLE:   Service %d: %s\n", i, advertisedDevice->getServiceUUID(i).toString().c_str());
      if(advertisedDevice->isAdvertisingService(NimBLEUUID("1812")))
      {
          Serial.println("BLE: Found remote, stopping scan");
          /** stop scan before connecting */
          NimBLEDevice::getScan()->stop();
          /** Save the device reference in a global for the client to use*/
          g_remote_device = advertisedDevice;
          /** Ready to connect now */
          g_do_nimble_connect = true;
      }          
    }
};

void scanEndedCB(NimBLEScanResults results){
    Serial.println("BLE: Scan Ended");
}

void setup() {
  Serial.begin(115200);
  Serial.println("BLE: Scanning...");

  NimBLEDevice::setScanFilterMode(CONFIG_BTDM_SCAN_DUPL_TYPE_DEVICE);


  NimBLEDevice::setScanDuplicateCacheSize(200);

  NimBLEDevice::init("");

  NimBLEScan* pBLEScan = NimBLEDevice::getScan(); //create new scan
  // Set the callback for when devices are discovered, no duplicates.
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), false);
  pBLEScan->setActiveScan(true); // Set active scanning, this will get more data from the advertiser.
  pBLEScan->setInterval(45); // How often the scan occurs / switches channels; in milliseconds,
  pBLEScan->setWindow(15);  // How long to scan during the interval; in milliseconds.
  pBLEScan->setMaxResults(0); // do not store the scan results, use callback only.
}

/** Notification / Indication receiving handler callback */
void notifyCB(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify)
{
    std::stringstream ss;
    ss << ((isNotify == true) ? "BLE: Notification" : "BLE Indication");
    ss << " from " << std::string(pRemoteCharacteristic->getRemoteService()->getClient()->getPeerAddress());
    /** NimBLEAddress and NimBLEUUID have std::string operators */
    ss << " svc=" << std::string(pRemoteCharacteristic->getRemoteService()->getUUID());
    ss << " chr=" << std::string(pRemoteCharacteristic->getUUID());
    ss << " len=" << length << " data="; 
    for (size_t i = 0; i < length; i++)
      ss << std::setfill('0') << std::setw(2) << std::hex << (int)pData[i] << ",";
    
    Serial.println(ss.str().c_str());
}

/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        */
class ClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* pClient) {
        Serial.println("BLE: Connected");
        /** After connection we should change the parameters if we don't need fast response times.
         *  These settings are 150ms interval, 0 latency, 450ms timout.
         *  Timeout should be a multiple of the interval, minimum is 100ms.
         *  I find a multiple of 3-5 * the interval works best for quick response/reconnect.
         *  Min interval: 120 * 1.25ms = 150, Max interval: 120 * 1.25ms = 150, 0 latency, 60 * 10ms = 600ms timeout
         */
        //pClient->updateConnParams(120,120,0,60);
    };

    void onDisconnect(NimBLEClient* pClient) {
         Serial.printf("BLE: Disconnected %s\n", pClient->getPeerAddress().toString().c_str());
         // don't do here, done in loop
         //NimBLEDevice::getScan()->start(0, scanEndedCB);
    };

    /** Called when the peripheral requests a change to the connection parameters.
     *  Return true to accept and apply them or false to reject and keep
     *  the currently used parameters. Default will return true.
     */
    bool onConnParamsUpdateRequest(NimBLEClient* pClient, const ble_gap_upd_params* params) {
        if(params->itvl_min < 24) { /** 1.25ms units */
            Serial.printf("onConnParams returning false 1: %d\n", params->itvl_min);
            return false;
        } else if(params->itvl_max > 40) { /** 1.25ms units */
            Serial.printf("onConnParams returning false 2\n");
            return false;
        } else if(params->latency > 2) { /** Number of intervals allowed to skip */
            Serial.printf("onConnParams returning false 3\n");
            return false;
        } else if(params->supervision_timeout > 100) { /** 10ms units */
            Serial.printf("onConnParams returning false 4\n");
            return false;
        }
        Serial.printf("onConnParams returning true");
        return true;
    };

    /********************* Security handled here **********************
    ****** Note: these are the same return values as defaults ********/
    uint32_t onPassKeyRequest(){
        Serial.println("BLE: Client Passkey Request");
        /** return the passkey to send to the server */
        return 123456;
    };

    bool onConfirmPIN(uint32_t pass_key){
        Serial.print("BLE: The passkey YES/NO number: ");
        Serial.println(pass_key);
    /** Return false if passkeys don't match. */
        return true;
    };

    /** Pairing process complete, we can check the results in ble_gap_conn_desc */
    void onAuthenticationComplete(ble_gap_conn_desc* desc){
        if(!desc->sec_state.encrypted) {
            Serial.println("BLE: Encrypt connection failed - disconnecting");
            /** Find the client with the connection handle provided in desc */
            NimBLEDevice::getClientByID(desc->conn_handle)->disconnect();
            return;
        }
    };
};


static ClientCallbacks clientCB;


bool connect_ble(NimBLEAdvertisedDevice* advDevice) 
{
    NimBLEClient* pClient = nullptr;

    /** Check if we have a client we should reuse first **/
    if(NimBLEDevice::getClientListSize()) {
        /** Special case when we already know this device, we send false as the
         *  second argument in connect() to prevent refreshing the service database.
         *  This saves considerable time and power.
         */
        pClient = NimBLEDevice::getClientByPeerAddress(advDevice->getAddress());
        if(pClient){
            if(!pClient->connect(advDevice, false)) {
                Serial.println("BLE: Reconnect failed");
                return false;
            }
            Serial.println("BLE: Reconnected client");
        }
        /** We don't already have a client that knows this device,
         *  we will check for a client that is disconnected that we can use.
         */
        else {
            pClient = NimBLEDevice::getDisconnectedClient();
        }
    }

    /** No client to reuse? Create a new one. */
    if(!pClient) {
        if(NimBLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS) {
            Serial.println("BLE: Max clients reached - no more connections available");
            return false;
        }

        pClient = NimBLEDevice::createClient();

        Serial.println("BLE: New client created");

        pClient->setClientCallbacks(&clientCB, false);
        /** Set initial connection parameters: These settings are 15ms interval, 0 latency, 120ms timout.
         *  These settings are safe for 3 clients to connect reliably, can go faster if you have less
         *  connections. Timeout should be a multiple of the interval, minimum is 100ms.
         *  Min interval: 12 * 1.25ms = 15, Max interval: 12 * 1.25ms = 15, 0 latency, 51 * 10ms = 510ms timeout
         */
        pClient->setConnectionParams(12,12,0,51);
        /** Set how long we are willing to wait for the connection to complete (seconds), default is 30. */
        pClient->setConnectTimeout(5);


        if (!pClient->connect(advDevice)) {
            /** Created a client but failed to connect, don't need to keep it as it has no data */
            NimBLEDevice::deleteClient(pClient);
            Serial.println("BLE: Failed to connect, deleted client");
            return false;
        }
    }

    if(!pClient->isConnected()) {
        if (!pClient->connect(advDevice)) {
            Serial.println("BLE: Failed to connect");
            return false;
        }
    }

    Serial.print("BLE: Connected to: ");
    Serial.print(pClient->getPeerAddress().toString().c_str());
    Serial.print("  RSSI: ");
    Serial.print(pClient->getRssi());
    Serial.print("  STR: ");
    Serial.print(pClient->toString().c_str());

    /** Now we can read/write/subscribe the charateristics of the services we are interested in */

        //pChr = pSvc->getCharacteristic("F00D");
        //pDsc = pChr->getDescriptor(NimBLEUUID("C01D"));
        
    NimBLERemoteService* pSvc = pClient->getService("1812");
    if (!pSvc) {
      Serial.println("BLE: 1812 service not found.");
      return false;
    }

    auto chrs = pSvc->getCharacteristics(true);
    Serial.printf("BLE: chars: %d\n", chrs->size());
    for(auto* pChr : *chrs) {
      Serial.printf("BLE:  Ch: %s\n", pChr->toString().c_str());

      if(pChr->canNotify()) {
          Serial.println("BLE:    subscribed notify");
          if(!pChr->subscribe(true, notifyCB, true)) {
              /** Disconnect if subscribe failed */
              Serial.println("BLE:    failed subscribed notify");
              pClient->disconnect();
              return false;
          }
      }
      else if(pChr->canIndicate()) {
          /** Send false as first argument to subscribe to indications instead of notifications */
          Serial.println("BLE:    subscribed indicate");
          if(!pChr->subscribe(false, notifyCB, true)) {
              /** Disconnect if subscribe failed */
              Serial.println("BLE:    failed subscribed indicate");
              pClient->disconnect();
              return false;
          }
      }
      

    }

    Serial.println("BLE:Done with this device!");
    return true;
}

void loop() {

  if (g_last_numble_scan == 0 || millis() - g_last_numble_scan > 2000)
  {
    // If an error occurs that stops the scan, it will be restarted here.
    if(NimBLEDevice::getScan()->isScanning() == false) {
        // Start scan with: duration = 0 seconds(forever), no scan end callback, not a continuation of a previous scan.
        Serial.println("BLE: starting scan");
        g_last_numble_scan = millis();
        NimBLEDevice::getScan()->start(0, scanEndedCB, false);
    }
  }

  if (g_do_nimble_connect)
  {
    g_do_nimble_connect = false;
    if(connect_ble(g_remote_device)) {
        Serial.println("BLE: Successfully connected to remote");
    } else {
        Serial.println("BLE: Failed to connect to remote");
    }
  }

  
}
