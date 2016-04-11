
#option(ompss "ompss" OFF)

if(ompss MATCHES ON)

#	set(CMAKE_CXX_COMPILER g++)
#	set(CMAKE_C_COMPILER gcc)

	SET (RTTL_SOURCES 
			BVH/BVH
			BVH/Builder/Builder
			BVH/Builder/Sweep
			BVH/Builder/BinnedAllDims
			BVH/Builder/BinnedAllDimsSaveSpace
			BVH/Builder/OnDemandBuilder
			common/MapOptions
			API/ISG
	)
	message("### OMPSS Enabled ###")
else(ompss MATCHES ON)
	SET (RTTL_SOURCES 
			BVH/BVH
			BVH/Builder/Builder
			BVH/Builder/Sweep
			BVH/Builder/BinnedAllDims
			BVH/Builder/BinnedAllDimsSaveSpace
			BVH/Builder/OnDemandBuilder
			common/RTThread
			common/MapOptions
			API/ISG
	)
	message("### Threads Enabled ###")
endif(ompss MATCHES ON)

ADD_LIBRARY(RTTL STATIC ${RTTL_SOURCES})
TARGET_LINK_LIBRARIES(RTTL stdc++ pthread rt)
INSTALL(TARGETS RTTL LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

#ADD_EXECUTABLE(test_rtvec test/TestRTVec/TestRTVec)
#TARGET_LINK_LIBRARIES(TestRTVec stdc++)

# ADD_EXECUTABLE(TestOptions test/TestOptions/TestOptions)
# TARGET_LINK_LIBRARIES(TestOptions RTTL)
 
# ADD_EXECUTABLE(test_newmesh test/api_newmesh/newmesh)
# TARGET_LINK_LIBRARIES(test_newmesh RTTL)

# ADD_EXECUTABLE(test_vertex_conversion test/api_vertex_conversion/vertex_conversion)
# TARGET_LINK_LIBRARIES(test_vertex_conversion RTTL)

