#include "OBJMeshLoader.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "StringHash.hpp"

using std::string;
using std::vector;


const char OBJFaceIndicator = 'f';
const char OBJNormalIndicator = 'n';
const char OBJPositionIndicator = 'p';
const char OBJTexelIndicator = 't';
const char OBJVertexIndicator = 'v';

const unsigned int OBJCommentCommandHash = hash("#");
const unsigned int OBJFaceCommandHash = hash("f");
const unsigned int OBJGroupNameCommandHash = hash("g");
const unsigned int OBJSmoothingGroupCommandHash = hash("s");
const unsigned int OBJUseMaterialCommandHash = hash("usemtl");
const unsigned int OBJVertexCommandHash = hash("v");
const unsigned int OBJVertexNormalCommandHash = hash("vn");
const unsigned int OBJVertexTextureCommandHash = hash("vt");


struct AbstractVertex
{
	unsigned int NormalIndex;
	unsigned int PositionIndex;
	unsigned int TexelIndex;

	bool operator==(const AbstractVertex& other)
	{
		bool areEqual = NormalIndex == other.NormalIndex
			&& PositionIndex == other.PositionIndex
			&& TexelIndex == other.TexelIndex;
		return areEqual;
	}
};

struct OBJTriangle
{
	AbstractVertex Vertices[3];
};

struct UnstructuredMeshData
{
	vector<OBJTriangle> Faces;
	vector<float3> Normals;
	vector<float3> Positions;
	vector<float3> Texels;
};

struct CompactifiedMeshData
{
	vector<unsigned int> Indices;
	vector<AbstractVertex> Vertices;
};

struct NormalizedMeshData
{
	unsigned int* Indices;
	OBJVertex* Vertices;
};


// you'd think that it wouldnt be that hard for someone to make a C++ compiler that doesnt require forward definitions...
CompactifiedMeshData CompactifyUnstructuredMeshData(UnstructuredMeshData unstructuredMeshData);
void CompareAndStoreVertexData(CompactifiedMeshData& result, AbstractVertex faceVertex);
vector<OBJVertex> ConvertAbstractVerticesToOBJVertices(const vector<AbstractVertex>& abstractVertices, UnstructuredMeshData unstructuredMeshData);
NormalizedMeshData NormalizeStructuredMeshData(UnstructuredMeshData unstructuredMeshData, CompactifiedMeshData compactifiedMeshData);
UnstructuredMeshData ReadUnstructuredMeshDataFromFile(string filePath);


CompactifiedMeshData CompactifyUnstructuredMeshData(UnstructuredMeshData unstructuredMeshData)
{
	CompactifiedMeshData result = {};
	for (unsigned int faceIndex = 0; faceIndex < unstructuredMeshData.Faces.size(); ++faceIndex)
	{
		OBJTriangle face = unstructuredMeshData.Faces[faceIndex];
		for (unsigned int faceVertexIndex = 0; faceVertexIndex < 3; ++faceVertexIndex)
		{
			AbstractVertex faceVertex = face.Vertices[faceVertexIndex];
			CompareAndStoreVertexData(result, faceVertex);
		}
	}
	return result;
}

void CompareAndStoreVertexData(CompactifiedMeshData& result, AbstractVertex faceVertex)
{
	bool isVertexUnique = true;
	unsigned int indexOfVertexInResult = 0;

	for (unsigned int unsortedVertexIndex = 0; unsortedVertexIndex < result.Vertices.size(); ++unsortedVertexIndex)
	{
		AbstractVertex unsortedVertex = result.Vertices[unsortedVertexIndex];
		if (faceVertex == unsortedVertex)
		{
			isVertexUnique = false;
			indexOfVertexInResult = unsortedVertexIndex;
			break;
		}
	}

	if (isVertexUnique)
	{
		indexOfVertexInResult = result.Vertices.size();
		result.Vertices.push_back(faceVertex);
	}

	result.Indices.push_back(indexOfVertexInResult);
}

vector<OBJVertex> ConvertAbstractVerticesToOBJVertices(const vector<AbstractVertex>& abstractVertices, UnstructuredMeshData unstructuredMeshData)
{
	vector<OBJVertex> objVertices;
	for (unsigned int i = 0; i < abstractVertices.size(); ++i)
	{
		AbstractVertex abstractVertex = abstractVertices[i];
		OBJVertex objVertex =
		{
			.Normal = unstructuredMeshData.Normals[abstractVertex.NormalIndex],
			.Position = unstructuredMeshData.Positions[abstractVertex.PositionIndex],
			.Texel = unstructuredMeshData.Texels[abstractVertex.TexelIndex],
		};
		objVertices.push_back(objVertex);
	}
	return objVertices;
}

NormalizedMeshData NormalizeStructuredMeshData(UnstructuredMeshData unstructuredMeshData, CompactifiedMeshData compactifiedMeshData)
{
	NormalizedMeshData result = {};

	result.Indices = new unsigned int[compactifiedMeshData.Indices.size()];
	std::copy(compactifiedMeshData.Indices.begin(), compactifiedMeshData.Indices.end(), result.Indices);

	vector<OBJVertex> objVertices = ConvertAbstractVerticesToOBJVertices(compactifiedMeshData.Vertices, unstructuredMeshData);
	result.Vertices = new OBJVertex[compactifiedMeshData.Vertices.size()];
	std::copy(objVertices.begin(), objVertices.end(), result.Vertices);

	return result;
}

UnstructuredMeshData ReadUnstructuredMeshDataFromFile_old(string filePath)
{
	UnstructuredMeshData result = {};

	std::ifstream inputFileStream(filePath, std::ios::in);
	if (!inputFileStream.is_open())
	{
		_RPTN(0, "Could not open OBJ file to load data\n", NULL);
	}

	char readLineInto[100];
	while (true)
	{
		inputFileStream.getline(readLineInto, 100, '\n');
		if (inputFileStream.eof())
		{
			break;
		}

		char extractVerticesFrom[100];
		strcpy_s(extractVerticesFrom, readLineInto);

		char separators[] = " ";
		char* nextToken = nullptr;
		char* token = strtok_s(extractVerticesFrom, separators, &nextToken);

		vector<char*> tokens;
		while (token != NULL)
		{
			tokens.push_back(token);
			token = strtok_s(NULL, separators, &nextToken);
		}

		char objLineTypeIndicator = readLineInto[0];
		switch (objLineTypeIndicator)
		{
			case OBJVertexIndicator:
			{
				// convert tokens into vertex data; first token is line identifier and is ignored
				switch (tokens[0][1])
				{
					case OBJNormalIndicator:
					{
						float3 normal =
						{
							.x = strtof(tokens[1], nullptr),
							.y = strtof(tokens[2], nullptr),
							.z = strtof(tokens[3], nullptr),
						};
						result.Normals.push_back(normal);
						break;
					}

					case OBJPositionIndicator:
					default:
					{
						float3 position =
						{
							.x = strtof(tokens[1], nullptr),
							.y = strtof(tokens[2], nullptr),
							.z = strtof(tokens[3], nullptr),
						};
						result.Positions.push_back(position);
						break;
					}

					case OBJTexelIndicator:
					{
						bool doesTexelHaveZValue = tokens.size() > 3;
						float3 texel =
						{
							.x = strtof(tokens[1], nullptr),
							.y = strtof(tokens[2], nullptr),
							.z = (doesTexelHaveZValue ? strtof(tokens[3], nullptr) : 0.0f),
						};
						result.Texels.push_back(texel);
						break;
					}
				}
				break;
			}

			case OBJFaceIndicator:
			{
				// convert tokens into vertices
				vector<AbstractVertex> tokenVertices;
				char faceSeparators[] = "/";
				for (unsigned int i = 1; i < tokens.size(); ++i)
				{
					char extractFacesFrom[100];
					strcpy_s(extractFacesFrom, tokens[i]);

					// split token into individual values
					vector<char*> faceTokens;
					token = strtok_s(extractFacesFrom, faceSeparators, &nextToken);
					while (token != NULL)
					{
						faceTokens.push_back(token);
						token = strtok_s(NULL, faceSeparators, &nextToken);
					}

					// convert values and add to list; raw values are 1-based, need to subtract 1 from each to make them 0-based
					AbstractVertex vertex;
					vertex.PositionIndex = strtoul(faceTokens[0], nullptr, 10) - 1;
					vertex.TexelIndex = strtoul(faceTokens[1], nullptr, 10) - 1;
					vertex.NormalIndex = strtoul(faceTokens[2], nullptr, 10) - 1;
					tokenVertices.push_back(vertex);
				}

				OBJTriangle face =
				{
					tokenVertices[0],
					tokenVertices[1],
					tokenVertices[2],
				};
				result.Faces.push_back(face);

				bool isFaceQuad = tokens.size() > 4;
				if (isFaceQuad)
				{
					face =
					{
						tokenVertices[2],
						tokenVertices[3],
						tokenVertices[0],
					};
					result.Faces.push_back(face);
				}
				break;
			}

			default:
				break;
		}
	}

	inputFileStream.close();
	return result;
}

UnstructuredMeshData ReadUnstructuredMeshDataFromFile(string filePath)
{
	UnstructuredMeshData result = {};

	std::ifstream fileIn(filePath);
	for (string line; getline(fileIn, line); )
	{
		std::istringstream lineStringStream(line);
		vector<string> tokens;
		for (string token; getline(lineStringStream, token, ' '); )
		{
			tokens.push_back(token);
		}

		string command = tokens.size() > 0 ? tokens[0] : "";
		switch (hash(command))
		{
			case OBJFaceCommandHash:
			{
				// convert tokens into vertices
				vector<AbstractVertex> tokenVertices;
				for (unsigned int i = 1; i < tokens.size(); ++i)
				{
					std::istringstream faceDataStringStream(tokens[i]);
					vector<string> faceTokens;
					for (string token; getline(faceDataStringStream, token, '/'); )
					{
						faceTokens.push_back(token);
					}

					// convert values and add to list; raw values are 1-based, need to subtract 1 from each to make them 0-based
					unsigned int normalTokenIndex = 2;
					unsigned int positionTokenIndex = 0;
					unsigned int texelTokenIndex = 1;
					AbstractVertex vertex =
					{
						.NormalIndex = strtoul(faceTokens[normalTokenIndex].c_str(), nullptr, 10) - 1,
						.PositionIndex = strtoul(faceTokens[positionTokenIndex].c_str(), nullptr, 10) - 1,
						.TexelIndex = strtoul(faceTokens[texelTokenIndex].c_str(), nullptr, 10) - 1,
					};
					tokenVertices.push_back(vertex);
				}

				OBJTriangle face =
				{
					tokenVertices[0],
					tokenVertices[1],
					tokenVertices[2],
				};
				result.Faces.push_back(face);

				bool isFaceQuad = tokens.size() > 4;
				if (isFaceQuad)
				{
					face =
					{
						tokenVertices[2],
						tokenVertices[3],
						tokenVertices[0],
					};
					result.Faces.push_back(face);
				}
				break;
			}

			case OBJVertexCommandHash:
			{
				float3 position =
				{
					.x = strtof(tokens[1].c_str(), nullptr),
					.y = strtof(tokens[2].c_str(), nullptr),
					.z = strtof(tokens[3].c_str(), nullptr),
				};
				result.Positions.push_back(position);
				break;
			}

			case OBJVertexNormalCommandHash:
			{
				float3 normal =
				{
					.x = strtof(tokens[1].c_str(), nullptr),
					.y = strtof(tokens[2].c_str(), nullptr),
					.z = strtof(tokens[3].c_str(), nullptr),
				};
				result.Normals.push_back(normal);
				break;
			}

			case OBJVertexTextureCommandHash:
			{
				bool doesTexelHaveZValue = tokens.size() > 3;
				float3 texel =
				{
					.x = strtof(tokens[1].c_str(), nullptr),
					.y = strtof(tokens[2].c_str(), nullptr),
					.z = (doesTexelHaveZValue ? strtof(tokens[3].c_str(), nullptr) : 0.0f),
				};
				result.Texels.push_back(texel);
				break;
			}

			default:
				break;
		}
	}

	fileIn.close();
	return result;
}


OBJMesh OBJMeshLoader::LoadOBJMesh(string filePath)
{
	UnstructuredMeshData unstructuredMeshData = ReadUnstructuredMeshDataFromFile(filePath);
	CompactifiedMeshData compactifiedMeshData = CompactifyUnstructuredMeshData(unstructuredMeshData);
	NormalizedMeshData normalizedMeshData = NormalizeStructuredMeshData(unstructuredMeshData, compactifiedMeshData);
	OBJMesh result =
	{
		normalizedMeshData.Indices,
		compactifiedMeshData.Indices.size(),
		normalizedMeshData.Vertices,
		compactifiedMeshData.Vertices.size(),
	};
	return result;
}
