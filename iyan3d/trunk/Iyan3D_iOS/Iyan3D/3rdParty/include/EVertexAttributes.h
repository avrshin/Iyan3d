#ifndef __E_VERTEX_ATTRIBUTES_H_INCLUDED__
#define __E_VERTEX_ATTRIBUTES_H_INCLUDED__

namespace irr
{
	namespace video
	{
		
		//! Enumeration for all vertex attibutes there are.
		enum E_VERTEX_ATTRIBUTES
		{
			EVA_POSITION = 0,
			EVA_NORMAL,
			EVA_COLOR,
			EVA_TCOORD0,
			EVA_TCOORD1,
			EVA_TANGENT,
			EVA_BINORMAL,
			EVA_WEIGHT1,
			EVA_WEIGHT2,
			EVA_BONES1,
			EVA_BONES2,
			EVA_COUNT
		};
		
		//! Array holding the built in vertex attribute names
		const char* const sBuiltInVertexAttributeNames[] =
		{
			"inVertexPosition",
			"inVertexNormal",
			"inVertexColor",
			"inTexCoord0",
			"inTexCoord1",
			"inVertexTangent",
			"inVertexBinormal",
			"inVertexWeight1",
			"inVertexWeight2",
			"inVertexBones1",
			"inVertexBones2",
			0
		};
		
	} // end namespace video
} // end namespace irr

#endif //__E_VERTEX_ATTRIBUTES_H_INCLUDED__