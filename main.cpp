/*
 * SPDX-FileCopyrightText: Bosch Rexroth AG
 *
 * SPDX-License-Identifier: MIT
 */

#include <iostream>
#include <filesystem>
#include <csignal>
#include <thread>
#include <chrono>
#include <typeinfo>

#include "comm/datalayer/datalayer.h"
#include "comm/datalayer/datalayer_system.h"
#include "comm/datalayer/metadata_generated.h"

#include "ctrlx_datalayer_helper.h"

#include "sampleSchema_generated.h"

 // Remote debug: Uncomment '#define REMOTE_DEBUG_ENABLED', build and install debugable snap
 // Local debug: Set '#define REMOTE_DEBUG_ENABLED' under comment, rebuild and debug
 //#define REMOTE_DEBUG_ENABLED

 // Add some signal Handling so we are able to abort the program with sending sigint
static bool g_endProcess = false;

static void sigIntHandler(int signal)
{
  std::cout << "signal: " << signal << std::endl;
  g_endProcess = true;
}

using comm::datalayer::IProviderNode;

// Basic class Provider node interface for providing data to the system
class MyProviderNode : public IProviderNode
{
private:
  comm::datalayer::Variant m_data;

  /* Keep this comment section - it can be used as a sample for creating metadata programmatically.

  comm::datalayer::Variant _metaData;

  void createMetadata()
  {
    flatbuffers::FlatBufferBuilder builder;
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<comm::datalayer::Reference>>> references;

    auto emptyString = builder.CreateString("This is a Description");

    auto isFlatbuffers = m_data.getType() == comm::datalayer::VariantType::FLATBUFFERS;
    if (isFlatbuffers)
    {
      flatbuffers::Offset<comm::datalayer::Reference> vecReferences[] =
          {
              comm::datalayer::CreateReferenceDirect(builder, "readType", "types/sampleSchema/inertialValue"),
              comm::datalayer::CreateReferenceDirect(builder, "writeType", "types/sampleSchema/inertialValue"),
          };

      references = builder.CreateVectorOfSortedTables(vecReferences, 2);
    }

    // Set allowed operations
    comm::datalayer::AllowedOperationsBuilder allowedOperations(builder);
    allowedOperations.add_read(true);
    allowedOperations.add_write(true);
    allowedOperations.add_create(false);
    allowedOperations.add_delete_(false);
    auto operations = allowedOperations.Finish();

    // Create metadata
    comm::datalayer::MetadataBuilder metadata(builder);
    metadata.add_nodeClass(comm::datalayer::NodeClass_Variable);
    metadata.add_description(emptyString);
    metadata.add_descriptionUrl(emptyString);
    metadata.add_operations(operations);
    if (isFlatbuffers)
    {
      metadata.add_references(references);
    }
    auto metaFinished = metadata.Finish();
    builder.Finish(metaFinished);

    _metaData.shareFlatbuffers(builder);
  }
*/

public:
  MyProviderNode(comm::datalayer::Variant data)
    : m_data(data)
  {};

  virtual ~MyProviderNode() override {};

  // Create function of an object. Function will be called whenever a object should be created.
  virtual void onCreate(const std::string& address, const comm::datalayer::Variant* data, const comm::datalayer::IProviderNode::ResponseCallback& callback) override
  {
    callback(comm::datalayer::DlResult::DL_FAILED, nullptr);
  }

  // Read function of a node. Function will be called whenever a node should be read.
  virtual void onRead(const std::string& address, const comm::datalayer::Variant* data, const comm::datalayer::IProviderNode::ResponseCallback& callback) override
  {
    comm::datalayer::Variant dataRead;
    dataRead = m_data;
    callback(comm::datalayer::DlResult::DL_OK, &dataRead);
  }

  // Write function of a node. Function will be called whenever a node should be written.
  virtual void onWrite(const std::string& address, const comm::datalayer::Variant* data, const comm::datalayer::IProviderNode::ResponseCallback& callback) override
  {
    std::cout << "INFO onWrite " << address << std::endl;

    if (data->getType() != m_data.getType())
    {
      callback(comm::datalayer::DlResult::DL_TYPE_MISMATCH, nullptr);
    }

    m_data = *data;
    callback(comm::datalayer::DlResult::DL_OK, data);
  }

  // Remove function for an object. Function will be called whenever a object should be removed.
  virtual void onRemove(const std::string& address, const comm::datalayer::IProviderNode::ResponseCallback& callback) override
  {
    callback(comm::datalayer::DlResult::DL_FAILED, nullptr);
  }

  // Browse function of a node. Function will be called to determine children of a node.
  virtual void onBrowse(const std::string& address, const comm::datalayer::IProviderNode::ResponseCallback& callback) override
  {
    callback(comm::datalayer::DlResult::DL_FAILED, nullptr);
  }

  // Read function of metadata of an object. Function will be called whenever a node should be written.
  virtual void onMetadata(const std::string& address, const comm::datalayer::IProviderNode::ResponseCallback& callback) override
  {
    // Keep this comment! Can be used as sample creating metadata programmatically.
    // callback(comm::datalayer::DlResult::DL_OK, &_metaData);

    // Take metadata from metadata.mddb
    callback(comm::datalayer::DlResult::DL_FAILED, nullptr);
  }
};

int main()
{

#ifdef REMOTE_DEBUG_ENABLED
  std::cout << "Starting 'raise(SIGSTOP)', waiting for debugger.." << std::endl;
  raise(SIGSTOP);
  std::cout << "Debugger connected, continuing program..." << std::endl;
#endif

  std::string dlBasePath = "sdk/cpp/datalayer/provider/simple/";

  comm::datalayer::DatalayerSystem datalayerSystem;
  // Starts the ctrlX Data Layer system without a new broker because one broker is already running on ctrlX CORE
  datalayerSystem.start(false);

  std::cout << "INFO Register '" << dlBasePath << "' with these sub nodes 'myFlatbuffer', 'myFloat', 'myString' and 'myInt64'" << std::endl;

  comm::datalayer::IProvider* provider = getProvider(datalayerSystem); // ctrlX CORE (virtual)
  if (provider == nullptr)
  {
    // provider = getProvider(datalayerSystem, "10.0.2.2", "boschrexroth", "boschrexroth", 8443); // ctrlX COREvirtual with port forwarding
    provider = getProvider(datalayerSystem, "192.168.1.1", "boschrexroth", "boschrexroth", 443); // ctrlX CORE
  }

  if (provider == nullptr)
  {
    std::cout << "ERROR Getting provider connection failed." << std::endl;
    datalayerSystem.stop(false);
    return 1;
  }

  // Register a node as bool value
  std::string dlPath = dlBasePath + "myBool";
  comm::datalayer::Variant myBool;
  myBool.setValue(false);
  std::cout << "INFO Register node " << dlPath << std::endl;
  comm::datalayer::DlResult result = provider->registerNode(dlPath, new MyProviderNode(myBool));
  if (STATUS_FAILED(result))
  {
    std::cout << "WARN Register node " << dlPath << " failed with: " << result.toString() << std::endl;
  }

  // Register a node as a array-of-float32 for Postion
  float afloat[] = {-1.0, 0.0};
  std::string dlPathPosition = dlBasePath + "array-of-float32-Postion";
  comm::datalayer::Variant nodeposition;
  nodeposition.setValue(afloat);
  std::cout << "INFO Register node " << dlPathPosition << std::endl;
  result = provider->registerNode(dlPathPosition, new MyProviderNode(nodeposition));
  if (STATUS_FAILED(result))
  {
    std::cout << "WARN Register node " << dlPathPosition << " failed with: " << result.toString() << std::endl;
  }

  // Register a node as a array-of-float32 for Couple
  std::string dlPathCouple = dlBasePath + "array-of-float32-Couple";
  comm::datalayer::Variant nodecouple;
  nodecouple.setValue(afloat);
  std::cout << "INFO Register node " << dlPathCouple << std::endl;
  result = provider->registerNode(dlPathCouple, new MyProviderNode(nodecouple));
  if (STATUS_FAILED(result))
  {
    std::cout << "WARN Register node " << dlPathCouple << " failed with: " << result.toString() << std::endl;
  }

  // Register a node as a array-of-float32 for Vitesse
  std::string dlPathVitesse = dlBasePath + "array-of-float32-Vitesse";
  comm::datalayer::Variant nodevitesse;
  nodevitesse.setValue(afloat);
  std::cout << "INFO Register node " << dlPathVitesse << std::endl;
  result = provider->registerNode(dlPathVitesse, new MyProviderNode(nodevitesse));
  if (STATUS_FAILED(result))
  {
    std::cout << "WARN Register node " << dlPathVitesse << " failed with: " << result.toString() << std::endl;
  }

  int32_t myInt[] = {0,73};  // Register a node as a array-of-int32 for Time
  std::string dlPathTime = dlBasePath + "array-of-int32-Time";
  comm::datalayer::Variant nodetime;
  nodetime.setValue(myInt);
  std::cout << "INFO Register node " << dlPathTime << std::endl;
  result = provider->registerNode(dlPathTime, new MyProviderNode(nodetime));
  if (STATUS_FAILED(result))
  {
    std::cout << "WARN Register node " << dlPathTime << " failed with: " << result.toString() << std::endl;
  }

  // Register a node as a string for Drive
  std::string dlPathDrive = dlBasePath + "string-Drive";
  comm::datalayer::Variant nodedrive;
  nodedrive.setValue("No_Drive_chosen");
  std::cout << "INFO Register node " << dlPathDrive << std::endl;
  result = provider->registerNode(dlPathDrive, new MyProviderNode(nodedrive));
  if (STATUS_FAILED(result))
  {
    std::cout << "WARN Register node " << dlPathDrive << " failed with: " << result.toString() << std::endl;
  }
  // Prepare signal structure to interrupt the endless loop with ctrl + c
  std::signal(SIGINT, sigIntHandler);


  auto connectionString = getConnectionString(); // default: ctrlX CORE or ctrlX COREvirtual with Network Adpater
  std::cout << "INFO Creating ctrlX Data Layer client connection to " << connectionString << " ..." << std::endl;
  auto dataLayerClient = datalayerSystem.factory()->createClient(connectionString);
 
  std::cout << "INFO Running endless loop - end with Ctrl+C" << std::endl;
  std::vector<float> arfloatVitesse;
  std::vector<float> arfloatPosition;
  std::vector<float> arfloatCouple;
  std::vector<int32_t> arintTime;


  auto pathflag = "sdk/cpp/datalayer/provider/simple/myBool";
  auto pathdrive = "sdk/cpp/datalayer/provider/simple/string-Drive";
  comm::datalayer::Variant sDriveValue;


  while (g_endProcess == false)
  {
    // Read the flag
    comm::datalayer::Variant myBoolVariant;
    dataLayerClient->readSync(pathflag, &myBoolVariant);
    bool myBool = myBoolVariant;

    // Start reading
    bool bFirstOutLoop = false;
    // Démarre le chronomètre avant la boucle de lecture
    auto startTotal = std::chrono::high_resolution_clock::now();
    int32_t totalDurationMilliseconds = 0; // Variable pour stocker la durée totale en millisecondes

    if (myBool){
      // Reset le vecteur
      arfloatVitesse.clear();
      arfloatPosition.clear();
      arfloatCouple.clear();
      arintTime.clear();
      bFirstOutLoop = true;

      // Choix du drive à lire
      auto resultDrive = dataLayerClient->readSync(pathdrive, &sDriveValue);
      const char* valueAsString = sDriveValue; // Supposons que cela fonctionne comme prévu
      std::cout << "Choix du drive : " << valueAsString << std::endl;
      // Convertion valueAsString en std::string pour faciliter la concaténation
      std::string driveName(valueAsString);
      // Construction des chemins
      std::string pathVitesse = "fieldbuses/ethercat/master/instances/ethercatmaster/realtime_data/input/data/" + driveName + "/AT.Encoder_actual_values_ENC_1_Velocity";
      std::string pathPosition = "fieldbuses/ethercat/master/instances/ethercatmaster/realtime_data/input/data/" + driveName + "/AT.Position_feedback_value_1";
      std::string pathCouple = "fieldbuses/ethercat/master/instances/ethercatmaster/realtime_data/input/data/" + driveName + "/AT.Torque_Actual_Value";
      comm::datalayer::Variant rVitesse;
      auto resultVitesse = dataLayerClient->readSync(pathVitesse.c_str(), &rVitesse);
      comm::datalayer::Variant rPosition;
      auto resultPosition = dataLayerClient->readSync(pathPosition.c_str(), &rPosition);
      comm::datalayer::Variant rCouple;
      auto resultCouple = dataLayerClient->readSync(pathCouple.c_str(), &rCouple);

      while (myBool)
      {
          // Verification du flag d'enregistrement
          dataLayerClient->readSync(pathflag, &myBoolVariant);
          myBool = myBoolVariant; 
          // Synchronous read
          std::cout << "INFO Reading synchronously..." << std::endl;
          dataLayerClient->readSync(pathVitesse, &rVitesse);
          dataLayerClient->readSync(pathPosition, &rPosition);
          dataLayerClient->readSync(pathCouple, &rCouple);
          // Push in a list
          arfloatVitesse.push_back(rVitesse);
          arfloatPosition.push_back(rPosition);
          arfloatCouple.push_back(rCouple);
          // Push time
          auto stopTotal = std::chrono::high_resolution_clock::now();
          auto durationTotal = std::chrono::duration_cast<std::chrono::milliseconds>(stopTotal - startTotal);
          totalDurationMilliseconds = durationTotal.count(); 
          arintTime.push_back(totalDurationMilliseconds);
      }
    }
    // SORTIE LECTURE
    if (bFirstOutLoop) {
      // Ecriture variables 
      comm::datalayer::Variant myArrayValue;
      myArrayValue.setValue(arfloatVitesse);
      dataLayerClient->writeSync(dlPathVitesse, &myArrayValue);
      myArrayValue.setValue(arfloatPosition);
      dataLayerClient->writeSync(dlPathPosition, &myArrayValue);
      myArrayValue.setValue(arfloatCouple);
      dataLayerClient->writeSync(dlPathCouple, &myArrayValue);
      myArrayValue.setValue(arintTime); 
      dataLayerClient->writeSync(dlPathTime, &myArrayValue);
      bFirstOutLoop = false;
    }

    // Synchronous write to Array of Float64  -----------------------------------------------------------
    std::cout << "INFO Sleeping..." << std::endl;
    if (provider->isConnected() == false)
    {
      std::cout << "ERROR Datalayer connection broken!" << std::endl;
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  std::cout << "INFO Exiting application" << std::endl;
  if (isSnap())
  {
    std::cout << "INFO Restarting automatically" << std::endl;
  }

  provider->unregisterType("types/sampleSchema/inertialValue");

  provider->unregisterNode(dlBasePath + "myBool");
  provider->unregisterNode(dlBasePath + "array-of-float32-Postion");
  provider->unregisterNode(dlBasePath + "array-of-float32-Couple");
  provider->unregisterNode(dlBasePath + "array-of-float32-Vitesse");
  provider->unregisterNode(dlBasePath + "array-of-int32-Time");
  provider->unregisterNode(dlBasePath + "string-Drive");

  // Clean up datalayer instances so that process ends properly
  provider->stop();
  delete provider;

  datalayerSystem.stop(false); // Attention: Doesn't return if any provider or client instance is still runnning

  return g_endProcess ? 0 : 1;
}
