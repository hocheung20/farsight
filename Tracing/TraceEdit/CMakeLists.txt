project(TraceEdit)
QT4_WRAP_CPP( GUI_MOC_SRCS TraceView3D.h MergeModel.h NodeModel.h TraceModel.h CellTraceModel.h cellexport.h screenshot.h
	      OPTIONS -DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED )
  
set( TRACE_SRCS
  TraceBit.h		TraceBit.cxx
  TraceLine.h		TraceLine.cxx
  TraceGap.h		TraceGap.cxx
  TraceObject.h		TraceObject.cxx
  TraceModel.h		TraceModel.cxx
  MergeModel.h		MergeModel.cxx
  branchPT.h		BranchPT.cxx
  ImageActors.h		ImageActors.cxx
  CellTrace.h		CellTrace.cxx
  CellTraceModel.h	CellTraceModel.cxx
  vtkPlotEdges.h	vtkPlotEdges.cxx 
  TraceView3D.h		TraceView3D.cxx 
  cellexport.h		cellexport.cxx
  screenshot.h		screenshot.cxx
  GridActors.h		GridActors.cxx
  VolumeOfInterest.h	VolumeOfInterest.cxx
  FeatureRelation.h	FeatureRelation.cxx
  ConvexHull3D.h	ConvexHull3D.cxx
  StructuredObject.h	StructuredObject.cxx
  PadImageFilter.h	PadImageFilter.cxx
  NodeModel.h		NodeModel.cxx
  ${GUI_MOC_SRCS}
  ) 

set( TRACE_LIBS
    tiff
    ${ITK_LIBRARIES}
    ${VTK_LIBRARIES}
    ftkGUI
    ftkCommon
    ftkGraphs
    MCLR
    vnl
    ${QT_LIBRARIES} )
  
if( BUILD_SPD )	
  list( APPEND TRACE_LIBS SPD ) 
endif(BUILD_SPD)

if(BUILD_CLUSCLUS)
  list( APPEND TRACE_LIBS ClusClus_lib )
endif(BUILD_CLUSCLUS)

if(BUILD_VESSEL_TRACING)
	list(APPEND TRACE_LIBS VesselTracer_lib)
endif(BUILD_VESSEL_TRACING)

if(BUILD_GT_CLUSTERING)
	list(APPEND TRACE_LIBS ftkGTClustering_lib)
endif(BUILD_GT_CLUSTERING)

if(QtTestingFound)
  link_directories( ${QtTesting_LIBRARY_DIR} )
  add_definitions(-DUSE_QT_TESTING)
endif()

add_library( Trace STATIC ${TRACE_SRCS} )
target_link_libraries( Trace ${TRACE_LIBS} )

if(BUILD_TESTING)
  add_subdirectory(Testing)
endif(BUILD_TESTING)

add_farsight_executable(TraceEdit vtkTraceViewerDriver.cxx Trace)

if(APPLE)
  set(plugin_path ${FARSIGHT_BUNDLE_LOCATION}/${PROJECT_NAME}.app/Contents/plugins/imageformats)
  set(resources_path ${FARSIGHT_BUNDLE_LOCATION}/${PROJECT_NAME}.app/Contents/Resources)
else(APPLE)
  set(plugin_path bin/plugins/imageformats)
  set(resources_path bin)
endif(APPLE)

#install qt.conf & Qt plugins loaded by TraceEdit at runtime
install(FILES ${QT_QGIF_PLUGIN_RELEASE} DESTINATION ${plugin_path})
install(FILES ${QT_QICO_PLUGIN_RELEASE} DESTINATION ${plugin_path})
install(FILES ${QT_QJPEG_PLUGIN_RELEASE} DESTINATION ${plugin_path})
install(FILES ${QT_QMNG_PLUGIN_RELEASE} DESTINATION ${plugin_path})
install(FILES ${QT_QSVG_PLUGIN_RELEASE} DESTINATION ${plugin_path})
install(FILES ${QT_QTIFF_PLUGIN_RELEASE} DESTINATION ${plugin_path})
install(FILES ${Farsight_SOURCE_DIR}/InstallRules/qt.conf DESTINATION ${resources_path})
