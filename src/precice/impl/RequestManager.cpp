// Copyright (C) 2011 Technische Universitaet Muenchen
// This file is part of the preCICE project. For conditions of distribution and
// use, please see the license notice at http://www5.in.tum.de/wiki/index.php/PreCICE_License
#include "RequestManager.hpp"
#include "com/Communication.hpp"
#include "cplscheme/CouplingScheme.hpp"
#include "precice/impl/SolverInterfaceImpl.hpp"
#include "spacetree/Spacetree.hpp"

namespace precice {
namespace impl {

tarch::logging::Log RequestManager:: _log("precice::impl::RequestManager");

RequestManager:: RequestManager
(
  bool                  geometryMode,
  SolverInterfaceImpl&  solverInterfaceImpl,
  com::PtrCommunication clientServerCommunication,
  cplscheme::PtrCouplingScheme couplingScheme)
:
  _isGeometryMode(geometryMode),
  _interface(solverInterfaceImpl),
  _com(clientServerCommunication),
  _couplingScheme(couplingScheme),
  _lockServerToClient(-1)
{}

void RequestManager:: handleRequests()
{
  preciceTrace("handleRequest()");
  bool requestsLeft = true;
  int clientCommSize = _com->getRemoteCommunicatorSize();
  int clientCounter = 0;
  std::list<int> clientRanks;
  while (requestsLeft){
    int requestID = -1;
    int rankSender = -1;
    if (_lockServerToClient == -1){
      int anySender = com::Communication::ANY_SENDER;
      rankSender = _com->receive(requestID, anySender);
      preciceDebug("Received request ID " << requestID << " from rank "
                   << rankSender << " (any sender)");
    }
    else {
      rankSender = _com->receive(requestID, _lockServerToClient);
      _lockServerToClient = -1;
      preciceDebug("Received request ID " << requestID << " from rank "
                   << rankSender << " (locked)");
    }
    preciceCheck(requestID != -1, "handleRequest()",
                 "Receiving of request ID failed");

    switch (requestID){
    case REQUEST_INITIALIZE:
      preciceDebug("Request initialize by rank " << rankSender);
      clientCounter++;
      assertion2(clientCounter <= clientCommSize, clientCounter, clientCommSize);
      clientRanks.push_front(rankSender);
      if (clientCounter == clientCommSize){
        handleRequestInitialze(clientRanks);
        clientCounter = 0;
        clientRanks.clear();
      }
      break;
    case REQUEST_INITIALIZE_DATA:
      preciceDebug("Request initialize data by rank " << rankSender);
      clientCounter++;
      assertion2(clientCounter <= clientCommSize, clientCounter, clientCommSize);
      clientRanks.push_front(rankSender);
      if (clientCounter == clientCommSize){
        handleRequestInitialzeData(clientRanks);
        clientCounter = 0;
        clientRanks.clear();
      }
      break;
    case REQUEST_ADVANCE:
      preciceDebug("Request advance by rank " << rankSender);
      clientCounter++;
      assertion2(clientCounter <= clientCommSize, clientCounter, clientCommSize);
      clientRanks.push_front(rankSender);
      if (clientCounter == clientCommSize){
        handleRequestAdvance(clientRanks);
        clientCounter = 0;
        clientRanks.clear();
      }
      break;
    case REQUEST_FINALIZE:
      preciceDebug("Request finalize by rank " << rankSender);
      clientCounter++;
      assertion2(clientCounter <= clientCommSize, clientCounter, clientCommSize);
      if (clientCounter == clientCommSize){
        handleRequestFinalize();
        return;
      }
      break;
    case REQUEST_FULFILLED_ACTION:
      handleRequestFulfilledAction(rankSender);
      break;
    case REQUEST_INQUIRE_POSITION:
      handleRequestInquirePosition(rankSender);
      break;
    case REQUEST_INQUIRE_CLOSEST_MESH:
      handleRequestInquireClosestMesh(rankSender);
      break;
    case REQUEST_INQUIRE_VOXEL_POSITION:
      handleRequestInquireVoxelPosition(rankSender);
      break;
    case REQUEST_SET_MESH_VERTEX:
      handleRequestSetMeshVertex(rankSender);
      break;
    case REQUEST_SET_WRITE_POSITION:
      handleRequestSetWritePosition(rankSender);
      break;
    case REQUEST_SET_WRITE_POSITIONS:
      handleRequestSetWritePositions(rankSender);
      break;
    case REQUEST_GET_WRITE_POSITIONS:
      handleRequestGetWritePositions(rankSender);
      break;
    case REQUEST_GET_WRITE_IDS_FROM_POSITIONS:
      handleRequestGetWriteIDsFromPositions(rankSender);
      break;
    case REQUEST_GET_WRITE_NODES_SIZE:
      handleRequestGetWriteNodesSize(rankSender);
      break;
    case REQUEST_SET_READ_POSITION:
      handleRequestSetReadPosition(rankSender);
      break;
    case REQUEST_SET_READ_POSITIONS:
      handleRequestSetReadPositions(rankSender);
      break;
    case REQUEST_GET_READ_POSITIONS:
      handleRequestGetReadPositions(rankSender);
      break;
    case REQUEST_GET_READ_IDS_FROM_POSITIONS:
      handleRequestGetReadIDsFromPositions(rankSender);
      break;
    case REQUEST_GET_READ_NODES_SIZE:
      handleRequestGetReadNodesSize(rankSender);
      break;
    case REQUEST_SET_MESH_EDGE:
      handleRequestSetMeshEdge(rankSender);
      break;
    case REQUEST_SET_MESH_TRIANGLE:
      handleRequestSetMeshTriangle(rankSender);
      break;
    case REQUEST_SET_MESH_TRIANGLE_WITH_EDGES:
      handleRequestSetMeshTriangleWithEdges(rankSender);
      break;
    case REQUEST_SET_MESH_QUAD:
      handleRequestSetMeshQuad(rankSender);
      break;
    case REQUEST_SET_MESH_QUAD_WITH_EDGES:
      handleRequestSetMeshQuadWithEdges(rankSender);
      break;
    case REQUEST_WRITE_SCALAR_DATA:
      handleRequestWriteScalarData(rankSender);
      break;
    case REQUEST_WRITE_BLOCK_VECTOR_DATA:
      handleRequestWriteBlockVectorData(rankSender);
      break;
    case REQUEST_WRITE_VECTOR_DATA:
      handleRequestWriteVectorData(rankSender);
      break;
    case REQUEST_READ_SCALAR_DATA:
      handleRequestReadScalarData(rankSender);
      break;
    case REQUEST_READ_VETOR_DATA:
      handleRequestReadVectorData(rankSender);
      break;
    case REQUEST_READ_BLOCK_VECTOR_DATA:
      handleRequestReadBlockVectorData(rankSender);
      break;
    case REQUEST_MAP_WRITTEN_DATA:
      preciceDebug("Request map written data by rank " << rankSender);
      clientCounter++;
      assertion2(clientCounter <= clientCommSize, clientCounter, clientCommSize);
      clientRanks.push_front(rankSender);
      if (clientCounter == clientCommSize){
        handleRequestMapWrittenData(clientRanks);
        clientCounter = 0;
        clientRanks.clear();
      }
      break;
    case REQUEST_MAP_READ_DATA:
      preciceDebug("Request map read data by rank " << rankSender);
      clientCounter++;
      assertion2(clientCounter <= clientCommSize, clientCounter, clientCommSize);
      clientRanks.push_front(rankSender);
      if (clientCounter == clientCommSize){
        handleRequestMapReadData(clientRanks);
        clientCounter = 0;
        clientRanks.clear();
      }
      break;
    case REQUEST_EXPORT_MESH:
      handleRequestExportMesh(rankSender);
      break;
    case REQUEST_INTEGRATE_SCALAR_DATA:
      handleRequestIntegrateScalarData(rankSender);
      break;
    case REQUEST_INTEGRATE_VECTOR_DATA:
      handleRequestIntegrateVectorData(rankSender);
      break;
    case REQUEST_PING:
      //bool ping = true;
      _com->send(true, rankSender);
      break;
    default:
      preciceError("handleRequest()", "Unknown RequestID \"" << requestID << "\"");
      break;
    }
  }
}
void RequestManager:: requestPing()
{
  preciceTrace("requestPing()");
  _com->send(REQUEST_PING, 0);
  int dummy = 0;
  _com->receive(dummy, 0);
}

void RequestManager:: requestInitialize()
{
  preciceTrace("requestInitialze()");
  _com->send(REQUEST_INITIALIZE, 0);
  _couplingScheme->receiveState(_com, 0);
}

void RequestManager:: requestInitialzeData()
{
  preciceTrace("requestInitialzeData()");
  if (_isGeometryMode){
    preciceInfo("requestInitializeData()",
                "Skipping data initialization in geometry mode");
    return;
  }
  _com->send(REQUEST_INITIALIZE_DATA, 0);
  _couplingScheme->receiveState(_com, 0);
}

void RequestManager:: requestAdvance
(
  double dt )
{
  preciceTrace("requestAdvance()");
  _com->send(REQUEST_ADVANCE, 0);
  int ping;
  _com->receive(ping, 0);
  _com->send(dt, 0);
  _couplingScheme->receiveState(_com, 0);
}

void RequestManager:: requestFinalize()
{
  preciceTrace("requestFinalize()");
  _com->send(REQUEST_FINALIZE, 0);
}


void RequestManager:: requestFulfilledAction
(
  const std::string& action )
{
  preciceTrace("requestFulfilledAction()");
  _com->send(REQUEST_FULFILLED_ACTION, 0);
  _com->send(action, 0);
}

int RequestManager:: requestInquirePosition
(
  utils::DynVector&    point,
  const std::set<int>& meshIDs )
{
  preciceTrace2("requestInquirePosition()", point, meshIDs.size());
  _com->send(REQUEST_INQUIRE_POSITION, 0);
  _com->send(tarch::la::raw(point), point.size(), 0);
  _com->send((int)meshIDs.size(), 0);
  foreach (int id, meshIDs){
    _com->send(id, 0);
  }
  int position = spacetree::Spacetree::positionUndefined();
  _com->receive(position, 0);
  assertion1(position != spacetree::Spacetree::positionUndefined(), position);
  return position;
}

void RequestManager:: requestInquireClosestMesh
(
  utils::DynVector&    point,
  const std::set<int>& meshIDs,
  ClosestMesh&         closest )
{
  preciceTrace2("requestInquireClosestMesh()", point, meshIDs.size());
  _com->send(REQUEST_INQUIRE_CLOSEST_MESH, 0);
  _com->send(tarch::la::raw(point), point.size(), 0);
  _com->send((int)meshIDs.size(), 0 );
  foreach (int id, meshIDs){
    _com->send(id, 0);
  }
  int sizeMeshIDs = 0;
  _com->receive(sizeMeshIDs, 0);
  for (int i=0; i < sizeMeshIDs; i++){
    int id = -1;
    _com->receive(id, 0);
    assertion1(id != -1, id);
    closest.meshIDs().push_back(id);
  }
  int position = -1;
  _com->receive(position, 0);
  assertion1(position != -1, position);
  closest.setPosition(position);
  double distanceVector[_interface.getDimensions()];
  _com->receive(distanceVector, _interface.getDimensions(), 0);
  closest.setDistanceVector(distanceVector);
}

void RequestManager:: requestInquireVoxelPosition
(
  utils::DynVector&    voxelCenter,
  utils::DynVector&    voxelHalflengths,
  bool                 includeBoundaries,
  const std::set<int>& meshIDs,
  VoxelPosition&       voxelPosition )
{
  preciceTrace4("requestInquireVoxelPosition()", voxelCenter, voxelHalflengths,
                includeBoundaries, meshIDs.size());
  _com->send(REQUEST_INQUIRE_VOXEL_POSITION, 0);
  using tarch::la::raw;
  _com->send(raw(voxelCenter), voxelCenter.size(), 0);
  _com->send(raw(voxelHalflengths), voxelHalflengths.size(), 0);
  _com->send(includeBoundaries, 0);
  _com->send((int)meshIDs.size(), 0);
  if (meshIDs.size() > 0){
    tarch::la::DynamicVector<int> idVector(meshIDs.size());
    int i = 0;
    foreach (int id, meshIDs){
      idVector[i] = id;
      i ++;
    }
    _com->send(raw(idVector), (int)meshIDs.size(), 0);
  }

  // Receive results
  int position = -1;
  _com->receive(position, 0);
  assertion1(position != -1, position);
  voxelPosition.setPosition(position);
  int sizeResultIDs = -1;
  _com->receive(sizeResultIDs, 0);
  if (sizeResultIDs > 0){
    voxelPosition.meshIDs().resize(sizeResultIDs);
    int* rawMeshIDs = raw(voxelPosition.meshIDs());
    _com->receive(rawMeshIDs, sizeResultIDs, 0);
  }
}

int RequestManager:: requestSetMeshVertex
(
  int               meshID,
  utils::DynVector& position )
{
  preciceTrace ( "requestSetMeshVertex()" );
  _com->send ( REQUEST_SET_MESH_VERTEX, 0 );
  _com->send ( meshID, 0 );
  _com->send ( tarch::la::raw(position), position.size(), 0 );
  int index = -1;
  _com->receive ( index, 0 );
  return index;
}

int RequestManager:: requestSetWritePosition
(
  int               meshID,
  utils::DynVector& position )
{
  preciceTrace("requestSetWritePosition()");
  _com->send(REQUEST_SET_WRITE_POSITION, 0);
  _com->send(meshID, 0);
  _com->send(tarch::la::raw(position), position.size(), 0);
  int index = -1;
  _com->receive(index, 0);
  return index;
}

void RequestManager:: requestSetWritePositions
(
  int     meshID,
  int     size,
  double* positions,
  int*    ids )
{
  preciceTrace("requestSetWritePositions()");
  _com->send(REQUEST_SET_WRITE_POSITIONS, 0);
  _com->send(meshID, 0);
  _com->send(size, 0);
  _com->send(positions, size*_interface.getDimensions(), 0);
  _com->receive(ids, size, 0);
}

void RequestManager:: requestGetWritePositions
(
  int     meshID,
  int     size,
  int*    ids,
  double* positions )
{
  preciceTrace("requestGetWritePositions()");
  _com->send(REQUEST_GET_WRITE_POSITIONS, 0);
  _com->send(meshID, 0);
  _com->send(size, 0);
  _com->send(ids, size, 0);
  _com->receive(positions, size*_interface.getDimensions(), 0);
}

void RequestManager:: requestGetWriteIDsFromPositions
(
  int     meshID,
  int     size,
  double* positions,
  int*    ids )
{
  preciceTrace1("requestGetWriteIDsFromPositions()", size);
  _com->send(REQUEST_GET_WRITE_IDS_FROM_POSITIONS, 0);
  _com->send(meshID, 0);
  _com->send(size, 0);
  _com->send(positions, size*_interface.getDimensions(), 0);
  _com->receive(ids, size, 0);
}

int RequestManager:: requestGetWriteNodesSize
(
  int meshID )
{
  preciceTrace1("requestGetWriteNodesSize()", meshID);
  _com->send(REQUEST_GET_WRITE_NODES_SIZE, 0);
  _com->send(meshID, 0);
  int size = 0;
  _com->receive(size, 0);
  return size;
}

int RequestManager:: requestSetReadPosition
(
  int               meshID,
  utils::DynVector& position )
{
  preciceTrace("requestSetReadPosition()");
  _com->send(REQUEST_SET_READ_POSITION, 0);
  _com->send(meshID, 0);
  _com->send(tarch::la::raw(position), position.size(), 0);
  int index = -1;
  _com->receive(index, 0);
  return index;
}

void RequestManager:: requestSetReadPositions
(
  int     meshID,
  int     size,
  double* positions,
  int*    ids )
{
  preciceTrace("requestSetReadPositions()");
  _com->send(REQUEST_SET_READ_POSITIONS, 0);
  _com->send(meshID, 0);
  _com->send(size, 0);
  _com->send(positions, size*_interface.getDimensions(), 0);
  _com->receive(ids, size, 0);
}

void RequestManager:: requestGetReadPositions
(
  int     meshID,
  int     size,
  int*    ids,
  double* positions )
{
  preciceTrace("requestGetReadPositions()");
  _com->send(REQUEST_GET_READ_POSITIONS, 0);
  _com->send(meshID, 0);
  _com->send(size, 0);
  _com->send(ids, size, 0);
  _com->receive(positions, size*_interface.getDimensions(), 0);
}

void RequestManager:: requestGetReadIDsFromPositions
(
  int     meshID,
  int     size,
  double* positions,
  int*    ids )
{
  preciceTrace1("requestGetReadIDsFromPositions()", size);
  _com->send(REQUEST_GET_READ_IDS_FROM_POSITIONS, 0);
  _com->send(meshID, 0);
  _com->send(size, 0);
  _com->send(positions, size*_interface.getDimensions(), 0);
  _com->receive(ids, size, 0);
}

int RequestManager:: requestGetReadNodesSize
(
  int meshID )
{
  preciceTrace1("requestGetReadNodesSize()", meshID);
  _com->send(REQUEST_GET_READ_NODES_SIZE, 0);
  _com->send(meshID, 0);
  int size = 0;
  _com->receive(size, 0);
  return size;
}

int RequestManager:: requestSetMeshEdge
(
  int meshID,
  int firstVertexID,
  int secondVertexID )
{
  preciceTrace3("requestSetMeshEdge()", meshID, firstVertexID, secondVertexID);
  _com->send(REQUEST_SET_MESH_EDGE, 0);
  int data[3] = { meshID, firstVertexID, secondVertexID };
  _com->send(data, 3, 0);
  int createdEdgeID = -1;
  _com->receive(createdEdgeID, 0);
  return createdEdgeID;
}

void RequestManager:: requestSetMeshTriangle
(
  int meshID,
  int firstEdgeID,
  int secondEdgeID,
  int thirdEdgeID )
{
  preciceTrace4("requestSetMeshTriangle()", meshID, firstEdgeID, secondEdgeID,
                thirdEdgeID);
  _com->send(REQUEST_SET_MESH_TRIANGLE, 0);
  int data[4] = {meshID, firstEdgeID, secondEdgeID, thirdEdgeID};
  _com->send(data, 4, 0);
}

void RequestManager:: requestSetMeshTriangleWithEdges
(
  int meshID,
  int firstVertexID,
  int secondVertexID,
  int thirdVertexID )
{
  preciceTrace4("requestSetMeshTriangleWithEdges()", meshID, firstVertexID,
                secondVertexID, thirdVertexID);
  _com->send(REQUEST_SET_MESH_TRIANGLE_WITH_EDGES, 0);
  int data[4] = {meshID, firstVertexID, secondVertexID, thirdVertexID};
  _com->send(data, 4, 0);
}

void RequestManager:: requestSetMeshQuad
(
  int meshID,
  int firstEdgeID,
  int secondEdgeID,
  int thirdEdgeID,
  int fourthEdgeID )
{
  preciceTrace5("requestSetMeshQuad()", meshID, firstEdgeID, secondEdgeID,
                thirdEdgeID, fourthEdgeID);
  _com->send(REQUEST_SET_MESH_QUAD, 0);
  int data[5] = {meshID, firstEdgeID, secondEdgeID, thirdEdgeID, fourthEdgeID};
  _com->send(data, 5, 0);
}

void RequestManager:: requestSetMeshQuadWithEdges
(
  int meshID,
  int firstVertexID,
  int secondVertexID,
  int thirdVertexID,
  int fourthVertexID )
{
  preciceTrace5("requestSetMeshTriangleWithEdges()", meshID, firstVertexID,
                secondVertexID, thirdVertexID, fourthVertexID);
  _com->send(REQUEST_SET_MESH_QUAD_WITH_EDGES, 0);
  int data[5] = {meshID, firstVertexID, secondVertexID, thirdVertexID, fourthVertexID};
  _com->send(data, 5, 0);
}

void RequestManager:: requestWriteScalarData
(
  int    dataID,
  int    valueIndex,
  double value )
{
  preciceTrace("requestWriteScalarData()");
  _com->send(REQUEST_WRITE_SCALAR_DATA, 0);
  _com->send(dataID, 0);
  _com->send(valueIndex, 0);
  _com->send(value, 0);
}

void RequestManager:: requestWriteBlockVectorData (
  int     dataID,
  int     size,
  int*    valueIndices,
  double* values )
{
  preciceTrace1("requestWriteBlockVectorData()", dataID);
  _com->send(REQUEST_WRITE_BLOCK_VECTOR_DATA, 0);
  _com->send(dataID, 0);
  _com->send(size, 0);
  _com->send(valueIndices, size, 0);
  _com->send(values, size*_interface.getDimensions(), 0);
}

void RequestManager:: requestWriteVectorData
(
  int     dataID,
  int     valueIndex,
  double* value )
{
  preciceTrace ( "requestWriteVectorData()" );
  _com->send(REQUEST_WRITE_VECTOR_DATA, 0);
  _com->send(dataID, 0);
  _com->send(valueIndex, 0);
  _com->send(value, _interface.getDimensions(), 0);
}

void RequestManager:: requestReadScalarData
(
  int     dataID,
  int     valueIndex,
  double& value )
{
  preciceTrace("requestReadScalarData()");
  _com->send(REQUEST_READ_SCALAR_DATA, 0);
  _com->send(dataID, 0);
  _com->send(valueIndex, 0);
  _com->receive(value, 0);
}

void RequestManager:: requestReadBlockVectorData
(
  int     dataID,
  int     size,
  int*    valueIndices,
  double* values )
{
  preciceTrace2("requestReadBlockVectorData()", dataID, size);
  _com->send(REQUEST_READ_BLOCK_VECTOR_DATA, 0);
  _com->send(dataID, 0);
  _com->send(size, 0);
  _com->send(valueIndices, size, 0);
  _com->receive(values, size*_interface.getDimensions(), 0);
}

void RequestManager:: requestReadVectorData
(
  int     dataID,
  int     valueIndex,
  double* value )
{
  preciceTrace("requestReadVectorData()");
  _com->send(REQUEST_READ_VETOR_DATA, 0);
  _com->send(dataID, 0);
  _com->send(valueIndex, 0);
  _com->receive(value, _interface.getDimensions(), 0);
}

void RequestManager:: requestMapWrittenData
(
  int meshID )
{
  preciceTrace1("requestMapWrittenData()", meshID);
  _com->send(REQUEST_MAP_WRITTEN_DATA, 0);
  int ping;
  _com->receive(ping, 0);
  _com->send(meshID, 0);
}

void RequestManager:: requestMapReadData
(
  int meshID )
{
  preciceTrace1("requestMapReadData()", meshID);
  _com->send(REQUEST_MAP_READ_DATA, 0);
  int ping;
  _com->receive(ping, 0);
  _com->send(meshID, 0);
}

void RequestManager:: requestExportMesh
(
  const std::string& filenameSuffix,
  int                exportType )
{
  preciceTrace("requestExportMesh()");
  _com->send(REQUEST_EXPORT_MESH, 0);
  _com->send(filenameSuffix, 0);
  _com->send(exportType, 0);
}

void RequestManager:: requestIntegrateScalarData
(
  int     dataID,
  double& integratedValue )
{
  preciceTrace("requestIntegrateScalarData()");
  _com->send(REQUEST_INTEGRATE_SCALAR_DATA, 0);
  _com->send(dataID, 0);
  _com->receive(integratedValue, 0);
}

void RequestManager:: requestIntegrateVectorData
(
  int     dataID,
  double* integratedValue )
{
  preciceTrace("requestIntegrateVectorData()");
  _com->send(REQUEST_INTEGRATE_VECTOR_DATA, 0);
  _com->send(dataID, 0);
  _com->receive(integratedValue, _interface.getDimensions(), 0);
}

void RequestManager:: handleRequestInitialze
(
  const std::list<int>& clientRanks )
{
  preciceTrace("handleRequestInitialze()");
  _interface.initialize();
  foreach (int rank, clientRanks){
    _couplingScheme->sendState(_com, rank);
  }
}

void RequestManager:: handleRequestInitialzeData
(
  const std::list<int>& clientRanks )
{
  preciceTrace("handleRequestInitializeData()");
  _interface.initializeData();
  foreach (int rank, clientRanks){
    _couplingScheme->sendState(_com, rank);
  }
}

void RequestManager:: handleRequestAdvance
(
  const std::list<int>& clientRanks )
{
  preciceTrace("handleRequestAdvance()");
  std::list<int>::const_iterator iter = clientRanks.begin();
  int ping = 0;
  _com->send(ping, *iter);
  double oldDt;
  _com->receive(oldDt, *iter);
  iter++;
  for (; iter != clientRanks.end(); iter++){
    _com->send(ping, *iter);
    double dt;
    _com->receive(dt, *iter);
    preciceCheck(tarch::la::equals(dt, oldDt), "handleRequestAdvance()",
                 "Ambiguous timestep length when calling request advance from "
                 << "several processes!");
    oldDt = dt;
  }
  _interface.advance(oldDt);
  foreach (int rank, clientRanks){
    _couplingScheme->sendState(_com, rank);
  }
}

void RequestManager:: handleRequestFinalize()
{
  preciceTrace ( "handleRequestFinalize()" );
  _interface.finalize();
}

void RequestManager:: handleRequestFulfilledAction
(
  int rankSender )
{
  preciceTrace1("handleRequestFulfilledAction()", rankSender);
  std::string action;
  _com->receive(action, rankSender);
  _interface.fulfilledAction(action);
}

void RequestManager:: handleRequestInquirePosition
(
  int rankSender )
{
  preciceTrace1("handleRequestInquirePosition()", rankSender);
  // Receive input
  double point[_interface.getDimensions()];
  _com->receive(point, _interface.getDimensions(), rankSender);
  std::set<int> meshIDs;
  int sizeMeshIDs = -1;
  _com->receive(sizeMeshIDs, rankSender);
  assertion1(sizeMeshIDs >= 0, sizeMeshIDs);
  for (int i=0; i < sizeMeshIDs; i++){
    int id = -1;
    _com->receive(id, rankSender);
    assertion1(id >= 0, id);
    meshIDs.insert(id);
  }

  // Perform request
  int position = _interface.inquirePosition(point, meshIDs);

  // Send output
  _com->send(position, rankSender);
}

void RequestManager:: handleRequestInquireClosestMesh
(
  int rankSender )
{
  preciceTrace1("handleRequestInquireClosestMesh()", rankSender);
  // Receive input
  double point[_interface.getDimensions()];
  _com->receive(point, _interface.getDimensions(), rankSender);
  std::set<int> meshIDs;
  int size = -1;
  _com->receive(size, rankSender);
  assertion1(size >= 0, size);
  for (int i=0; i < size; i++){
    int id = -1;
    _com->receive(id, rankSender);
    assertion1(id >= 0, id);
    meshIDs.insert(id);
  }

  // Perform request
  ClosestMesh closest = _interface.inquireClosestMesh(point, meshIDs);

  // Send output
  _com->send((int)closest.meshIDs().size(), rankSender);
  foreach (int id, closest.meshIDs()){
    _com->send(id, rankSender);
  }
  _com->send(closest.position(), rankSender);
  double distanceVector[_interface.getDimensions()];
  for (int dim=0; dim < _interface.getDimensions(); dim++){
    distanceVector[dim] = closest.distanceVector()[dim];
  }
  _com->send(distanceVector, _interface.getDimensions(), rankSender);
}

void RequestManager:: handleRequestInquireVoxelPosition
(
  int rankSender )
{
  preciceTrace1("handleRequestInquireVoxelPosition()", rankSender);
  // Receive input
  double voxelCenter[_interface.getDimensions()];
  _com->receive(voxelCenter, _interface.getDimensions(), rankSender);
  double voxelHalflengths[_interface.getDimensions()];
  _com->receive(voxelHalflengths, _interface.getDimensions(), rankSender);
  bool includeBoundaries = false; // initialize to please compiler
  _com->receive(includeBoundaries, rankSender);
  int sizeMeshIDs = -1;
  std::set<int> meshIDs;
  _com->receive(sizeMeshIDs, rankSender);
  assertion1(sizeMeshIDs >= 0, sizeMeshIDs);
  if (sizeMeshIDs > 0){
    tarch::la::DynamicVector<int> idVector(sizeMeshIDs);
    _com->receive(tarch::la::raw(idVector), sizeMeshIDs, rankSender);
    for (int i=0; i < sizeMeshIDs; i++){
      meshIDs.insert(idVector[i]);
    }
  }
  // Perform request
  VoxelPosition content = _interface.inquireVoxelPosition(voxelCenter,
                          voxelHalflengths, includeBoundaries, meshIDs );

  // Send output
  _com->send(content.position(), rankSender);
  sizeMeshIDs = (int)content.meshIDs().size();
  _com->send(sizeMeshIDs, rankSender);
  if (sizeMeshIDs > 0){
    int* rawMeshIDs = tarch::la::raw(content.meshIDs());
    _com->send(rawMeshIDs, sizeMeshIDs, rankSender);
  }
}

void RequestManager:: handleRequestSetMeshVertex
(
  int rankSender )
{
  preciceTrace1("handleRequestSetMeshVertex()", rankSender);
  int meshID = -1;
  _com->receive(meshID, rankSender);
  double position[_interface.getDimensions()];
  _com->receive(position, _interface.getDimensions(), rankSender);
  int index = _interface.setMeshVertex(meshID, position);
  _com->send(index, rankSender);
}

void RequestManager:: handleRequestSetWritePosition
(
  int rankSender )
{
  preciceTrace1("handleRequestSetWritePosition()", rankSender);
  //com::PtrCommunication com = _accessor->getClientServerCommunication();
  int meshID = -1;
  _com->receive(meshID, rankSender);
  double position[_interface.getDimensions()];
  _com->receive(position, _interface.getDimensions(), rankSender);
  int index = _interface.setWritePosition(meshID, position);
//  if (_accessor->meshContext(meshID).writeMappingContext.isIncremental){
//    _lockServerToClient = rankSender;
//  }
  _com->send(index, rankSender);
}

void RequestManager:: handleRequestSetWritePositions
(
  int rankSender )
{
  preciceTrace1("handleRequestSetWritePositions()", rankSender);
  int meshID = -1;
  _com->receive(meshID, rankSender);
  int size = -1;
  _com->receive(size, rankSender);
  assertionMsg(size > 0, size);
  double* positions = new double[size*_interface.getDimensions()];
  _com->receive(positions, size*_interface.getDimensions(), rankSender);
  int* ids = new int[size];
  _interface.setWritePositions(meshID, size, positions, ids);
  _com->send(ids, size, rankSender);
  delete[] positions;
  delete[] ids;
}

void RequestManager:: handleRequestGetWritePositions
(
  int rankSender )
{
  preciceTrace1("handleRequestGetWritePositions()", rankSender);
  int meshID = -1;
  int size = -1;
  _com->receive(meshID, rankSender);
  _com->receive(size, rankSender);
  assertionMsg(size > 0, size);
  int* ids = new int[size];
  double* positions = new double[size*_interface.getDimensions()];
  _com->receive(ids, size, rankSender);
  _interface.getWritePositions(meshID, size, ids, positions);
  _com->send(positions, size*_interface.getDimensions(), rankSender);
  delete[] ids;
  delete[] positions;
}

void RequestManager:: handleRequestGetWriteIDsFromPositions
(
  int rankSender )
{
  preciceTrace1("handleRequestGetWriteIDsFromPositions()", rankSender);
  int meshID = -1;
  int size = -1;
  _com->receive(meshID, rankSender);
  _com->receive(size, rankSender);
  assertionMsg(size > 0, size);
  int* ids = new int[size];
  double* positions = new double[size*_interface.getDimensions()];
  _com->receive(positions, size*_interface.getDimensions(), rankSender);
  _interface.getWriteIDsFromPositions(meshID, size, positions, ids);
  _com->send(ids, size, rankSender);
  delete[] ids;
  delete[] positions;
}

void RequestManager:: handleRequestGetWriteNodesSize
(
  int rankSender )
{
  preciceTrace1("handleRequestGetWriteNodesSize()", rankSender);
  int meshID = -1;
  _com->receive(meshID, rankSender);
  int size = _interface.getWriteNodesSize(meshID);
  _com->send(size, rankSender);
}

void RequestManager:: handleRequestSetReadPosition
(
  int rankSender )
{
  preciceTrace1("handleRequestSetReadPosition()", rankSender);
  int meshID = -1;
  _com->receive (meshID, rankSender);
  double position[_interface.getDimensions()];
  _com->receive(position, _interface.getDimensions(), rankSender);
//  if (_accessor->meshContext(meshID).readMappingContext.isIncremental){
//    _lockServerToClient = rankSender;
//  }
  int index = _interface.setReadPosition(meshID, position);
  _com->send(index, rankSender);
}

void RequestManager:: handleRequestSetReadPositions
(
  int rankSender )
{
  preciceTrace1("handleRequestSetReadPositions()", rankSender);
  int meshID = -1;
  _com->receive(meshID, rankSender);
  int size = -1;
  _com->receive(size, rankSender);
  assertionMsg(size > 0, size);
  double* positions = new double[size*_interface.getDimensions()];
  _com->receive(positions, size*_interface.getDimensions(), rankSender);
  int* ids = new int[size];
  _interface.setReadPositions(meshID, size, positions, ids);
  _com->send(ids, size, rankSender);
  delete[] positions;
  delete[] ids;
}

void RequestManager:: handleRequestGetReadPositions
(
  int rankSender )
{
  preciceTrace1("handleRequestGetReadPositions()", rankSender);
  int meshID = -1;
  int size = -1;
  _com->receive(meshID, rankSender);
  _com->receive(size, rankSender);
  assertionMsg(size > 0, size);
  int* ids = new int[size];
  double* positions = new double[size*_interface.getDimensions()];
  _com->receive(ids, size, rankSender);
  _interface.getReadPositions(meshID, size, ids, positions);
  _com->send(positions, size*_interface.getDimensions(), rankSender);
  delete[] ids;
  delete[] positions;
}

void RequestManager:: handleRequestGetReadIDsFromPositions
(
  int rankSender )
{
  preciceTrace1("handleRequestGetReadIDsFromPositions()", rankSender);
  int meshID = -1;
  int size = -1;
  _com->receive(meshID, rankSender);
  _com->receive(size, rankSender);
  assertionMsg(size > 0, size);
  int* ids = new int[size];
  double* positions = new double[size*_interface.getDimensions()];
  _com->receive(positions, size*_interface.getDimensions(), rankSender);
  _interface.getReadIDsFromPositions(meshID, size, positions, ids);
  _com->send(ids, size, rankSender);
  delete[] ids;
  delete[] positions;
}

void RequestManager:: handleRequestGetReadNodesSize
(
  int rankSender )
{
  preciceTrace1("handleRequestGetReadNodesSize()", rankSender);
  int meshID = -1;
  _com->receive(meshID, rankSender);
  int size = _interface.getReadNodesSize(meshID);
  _com->send(size, rankSender);
}

void RequestManager:: handleRequestSetMeshEdge
(
  int rankSender )
{
  preciceTrace1("handleRequestSetMeshEdge()", rankSender);
  int data[3]; // 0: meshID, 1: firstVertexID, 2: secondVertexID
  _com->receive(data, 3, rankSender);
  int createEdgeID = _interface.setMeshEdge(data[0], data[1], data[2]);
  _com->send(createEdgeID, rankSender);
}

void RequestManager:: handleRequestSetMeshTriangle
(
  int rankSender )
{
  preciceTrace1("handleRequestSetMeshTriangle()", rankSender);
  int data[4]; // 0: meshID, 1,2,3: edge IDs
  _com->receive(data, 4, rankSender);
  _interface.setMeshTriangle(data[0], data[1], data[2], data[3]);
}

void RequestManager:: handleRequestSetMeshTriangleWithEdges
(
  int rankSender )
{
  preciceTrace1("handleRequestSetMeshTriangleWithEdges()", rankSender);
  int data[4]; // 0: meshID, 1,2,3: vertex IDs
  _com->receive(data, 4, rankSender);
  _interface.setMeshTriangleWithEdges(data[0], data[1], data[2], data[3]);
}

void RequestManager:: handleRequestSetMeshQuad
(
  int rankSender )
{
  preciceTrace1("handleRequestSetMeshQuad()", rankSender);
  int data[5]; // 0: meshID, 1,2,3,4: edge IDs
  _com->receive(data, 5, rankSender);
  _interface.setMeshQuad(data[0], data[1], data[2], data[3], data[4]);
}

void RequestManager:: handleRequestSetMeshQuadWithEdges
(
  int rankSender )
{
  preciceTrace1("handleRequestSetMeshQuadWithEdges()", rankSender);
  int data[5]; // 0: meshID, 1,2,3,4: vertex IDs
  _com->receive(data, 5, rankSender);
  _interface.setMeshQuadWithEdges(data[0], data[1], data[2], data[3], data[4]);
}

void RequestManager:: handleRequestWriteScalarData
(
  int rankSender )
{
  preciceTrace1("handleRequestWriteScalarData()", rankSender);
  int dataID = -1;
  _com->receive(dataID, rankSender);
  int index = -1;
  _com->receive(index, rankSender);
  double data;
  _com->receive(data, rankSender);
  _interface.writeScalarData(dataID, index, data);
}

void RequestManager:: handleRequestWriteBlockVectorData
(
  int rankSender )
{
  preciceTrace1("handleRequestWriteBlockVectorData()", rankSender);
  int dataID = -1;
  _com->receive(dataID, rankSender);
  int size = -1;
  _com->receive(size, rankSender);
  int* indices = new int[size];
  _com->receive(indices, size, rankSender);
  double* data = new double[size*_interface.getDimensions()];
  _com->receive(data, size*_interface.getDimensions(), rankSender);
  _interface.writeBlockVectorData(dataID, size, indices, data);
  delete[] indices;
  delete[] data;
}

void RequestManager:: handleRequestWriteVectorData
(
  int rankSender )
{
  preciceTrace1("handleRequestWriteVectorData()", rankSender);
  int dataID = -1;
  _com->receive(dataID, rankSender);
  int index = -1;
  _com->receive( index, rankSender);
  double data[_interface.getDimensions()];
  _com->receive(data, _interface.getDimensions(), rankSender);
  _interface.writeVectorData(dataID, index, data);
}

void RequestManager:: handleRequestReadScalarData
(
  int rankSender )
{
  preciceTrace1("handleRequestReadScalarData()", rankSender);
  int dataID = -1;
  _com->receive(dataID, rankSender);
  int index = -1;
  _com->receive(index, rankSender);
  double data;
  _interface.readScalarData(dataID, index, data);
  _com->send(data, rankSender); // Send back result
}

void RequestManager:: handleRequestReadBlockVectorData
(
  int rankSender )
{
  preciceTrace1("handleRequestReadBlockVectorData()", rankSender);
  int dataID = -1;
  _com->receive(dataID, rankSender);
  int size = -1;
  _com->receive(size, rankSender);
  int* indices = new int[size];
  _com->receive(indices, size, rankSender);
  double* data = new double[size*_interface.getDimensions()];
  _interface.readBlockVectorData(dataID, size, indices, data);
  _com->send(data, size*_interface.getDimensions(), rankSender);
  delete[] indices;
  delete[] data;
}

void RequestManager:: handleRequestReadVectorData
(
  int rankSender )
{
  preciceTrace1("handleRequestReadVectorData()", rankSender);
  int dataID = -1;
  _com->receive(dataID, rankSender);
  int index = -1;
  _com->receive(index, rankSender);
  double data[_interface.getDimensions()];
  _interface.readVectorData(dataID, index, data);
  _com->send(data, _interface.getDimensions(), rankSender);
}

void RequestManager:: handleRequestMapWrittenData
(
  const std::list<int>& clientRanks )
{
  preciceTrace("handleRequestMapWrittenData()");
  std::list<int>::const_iterator iter = clientRanks.begin();
  int ping = 0;
  _com->send(ping, *iter);
  int oldMeshID;
  _com->receive(oldMeshID, *iter);
  iter++;
  for (; iter != clientRanks.end(); iter++){
    _com->send(ping, *iter);
    int meshID;
    _com->receive(meshID, *iter);
    preciceCheck(meshID == oldMeshID, "handleRequestMapWrittenData()",
                 "Ambiguous mesh ID when calling map written data from "
                 << "several processes!");
    oldMeshID = meshID;
  }
  _interface.mapWrittenData(oldMeshID);
}

void RequestManager:: handleRequestMapReadData
(
  const std::list<int>& clientRanks )
{
  preciceTrace("handleRequestMapReadData()");
  std::list<int>::const_iterator iter = clientRanks.begin();
  int ping = 0;
  _com->send(ping, *iter);
  int oldMeshID;
  _com->receive(oldMeshID, *iter);
  iter++;
  for (; iter != clientRanks.end(); iter++){
    _com->send(ping, *iter);
    int meshID;
    _com->receive(meshID, *iter);
    preciceCheck(meshID == oldMeshID, "handleRequestMapReadData()",
                 "Ambiguous mesh IDs (" << meshID << " and " << oldMeshID
                 <<  ") when calling map read data from several processes!");
    oldMeshID = meshID;
  }
  _interface.mapReadData(oldMeshID);
}

void RequestManager:: handleRequestExportMesh
(
  int rankSender )
{
  preciceTrace1("handleRequestExportMesh()", rankSender);
  std::string filenameSuffix;
  _com->receive(filenameSuffix, rankSender);
  int exportType;
  _com->receive(exportType, rankSender);
  _interface.exportMesh(filenameSuffix, exportType);
}

void RequestManager:: handleRequestIntegrateScalarData
(
  int rankSender )
{
  preciceTrace1("handleRequestIntegrateScalarData()", rankSender);
  int dataID = -1;
  _com->receive(dataID, rankSender);
  double data = 0.0;
  _interface.integrateData(dataID, data);
  _com->send(data, rankSender);
}

void RequestManager:: handleRequestIntegrateVectorData
(
  int rankSender )
{
  preciceTrace1("handleRequestIntegrateVectorData()", rankSender);
  int dataID = -1;
  _com->receive(dataID, rankSender);
  double data[_interface.getDimensions()];
  _interface.integrateData(dataID, data);
  _com->send(data, _interface.getDimensions(), rankSender);
}

}} // namespace precice, impl