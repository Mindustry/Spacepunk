if( WIN32 )
	list(APPEND GAME_SOURCES
		"${CMAKE_CURRENT_SOURCE_DIR}/ico.o"
	)
endif()

list(APPEND GAME_SOURCES
	"${CMAKE_CURRENT_SOURCE_DIR}/Animation.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/AnimationState.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Asset.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Atlas.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/BBox.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Button.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Camera.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Chunk.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Client.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Character.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Component.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Console.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Cube.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Cubemap.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Dictionary.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Directory.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Editor.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Engine.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Entity.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/File.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Field.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Frame.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Game.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Generator.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Image.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Input.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Light.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Line3D.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Material.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Mesh.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Mixer.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Model.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Multimesh.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Net.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/NetSDL.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Packet.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Path.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Player.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Random.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Renderer.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/savepng.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Script.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Sector.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/SectorVertex.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/SectorWorld.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Server.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Shader.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/ShaderProgram.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Shadow.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Sound.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Speaker.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Text.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Texture.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Tile.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/TileWorld.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Voxel.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/World.cpp"
)

set(GAME_SOURCES ${GAME_SOURCES} PARENT_SCOPE)
