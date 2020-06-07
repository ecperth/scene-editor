// UWA CITS3003 Graphics 'n' Animation Tool Interface & Data Reader
// Part 2

// You shouldn't need to modify the code in this file, but feel free to.
// If you do, it would be good to mark your changes with comments.


// Load a model's scene by number from the models-textures directory via the Open Asset Importer
const aiScene* loadScene(int meshNumber) {
        char filename[256];
        sprintf_s(filename, "%s/model%d.x", dataDir, meshNumber);
        return aiImportFile(filename, aiProcessPreset_TargetRealtime_MaxQuality);
}

// Extract the boneIDs and boneWeights for the bones affecting each vertex in a mesh.  
// Each vertex has up to 4 bones - if there are more than 4, lower weighted bones are omitted.  
void getBonesAffectingEachVertex(aiMesh* mesh, GLint **boneIDs, GLfloat **boneWeights) {

    // Initialize all weights to 0.0
    for(unsigned int i=0; i < mesh->mNumVertices; i++)
        for(int j=0; j<4; j++)  
            boneWeights[i][j] = 0.0;

    if(mesh->mNumBones == 0) {  // No bones, so just use a single matrix (which should be the identity)
		// we are setting the weight of the first bone of each vertex to 1 
		// otherwise the shader bone matrix will evaluate to 0 and nothing will be visible.
        for(unsigned int i=0; i < mesh->mNumVertices; i++) {
				boneWeights[i][0] = 1.0f;
				boneIDs[i][0] = 0;
        }
        return;
    }   // endif     

	for (unsigned int boneID = 0; boneID < mesh->mNumBones; boneID++)    // loop through bone weight data 
		for (unsigned int weightID = 0; weightID < mesh->mBones[boneID]->mNumWeights; weightID++) {
			int VertexID = mesh->mBones[boneID]->mWeights[weightID].mVertexId;
			float Weight = mesh->mBones[boneID]->mWeights[weightID].mWeight;

			// Select the 4 largest weights via an insertion sort
			for (int slotID = 0; slotID < 4; slotID++)
				if (boneWeights[VertexID][slotID] < Weight) {
					for (int shuff = 3; shuff > slotID; shuff--) {
						boneWeights[VertexID][shuff] = boneWeights[VertexID][shuff - 1];
						boneIDs[VertexID][shuff] = boneIDs[VertexID][shuff - 1];
					}
					boneWeights[VertexID][slotID] = Weight;
					boneIDs[VertexID][slotID] = boneID;
					break;
				}
		}
} // end getBonesAffectingEachVertex()

// Parts of the following are broadly based on:
//     http://sourceforge.net/projects/assimp/forums/forum/817654/topic/3880745
//     http://ogldev.atspace.co.uk/www/tutorial38/tutorial38.html

// calculateAnimPose calculates the bone transformations for a mesh at a particular time in an animation (in scene)
// Each bone transformation is relative to the rest pose.
void calculateAnimPose(aiMesh* mesh, const aiScene* scene, int animNum, float poseTime, glm::mat4 *boneTransforms) {
//NOTE: Assimp uses row order matrices whereas GLM uses column order.
	// So we have to do some evasive work to make things good.

    if(mesh->mNumBones == 0 || animNum < 0) {				// animNum = -1 for no animation
			boneTransforms[0] = glm::mat4();				// so, just return a single identity matrix
			// See comments below re assimp vs GLM matrices
			//we can return GLM identity directly because transpose of identity matrix is itself
        return;
    }

	//std::cout << mesh->mNumBones << "  " << animNum  << std::endl;

    if(scene->mNumAnimations <= (unsigned int)animNum)    
        failInt("No animation with number:", animNum);

    aiAnimation *anim = scene->mAnimations[animNum];  // animNum = 0 for the first animation

    // Set transforms from bone channels 
    for(unsigned int chanID=0; chanID < anim->mNumChannels; chanID++) {
        aiNodeAnim *channel = anim->mChannels[chanID];        
        aiVector3D curPosition;
        aiQuaternion curRotation;   // interpolation of scaling purposefully left out for simplicity.

        // find the node which the channel affects
        aiNode* targetNode = scene->mRootNode->FindNode( channel->mNodeName );

        // find current positionKey
        size_t posIndex = 0;
        for(posIndex=0; posIndex+1 < channel->mNumPositionKeys; posIndex++)
            if( channel->mPositionKeys[posIndex + 1].mTime > poseTime )
                break;   // the next key lies in the future - so use the current key
            
        // This assumes that there is at least one key
        if(posIndex+1 == channel-> mNumPositionKeys)
             curPosition = channel->mPositionKeys[posIndex].mValue;  
        else {
            float t0 = channel->mPositionKeys[posIndex].mTime;   // Interpolate position/translation
            float t1 = channel->mPositionKeys[posIndex+1].mTime;
            float weight1 = (poseTime-t0)/(t1-t0);  

            curPosition = channel->mPositionKeys[posIndex].mValue * (1.0f - weight1) + 
                          channel->mPositionKeys[posIndex+1].mValue * weight1;
        }

        // find current rotationKey
        size_t rotIndex = 0;
        for(rotIndex=0; rotIndex+1 < channel->mNumRotationKeys; rotIndex++)
            if( channel->mRotationKeys[rotIndex + 1].mTime > poseTime )
                break;   // the next key lies in the future - so use the current key

        if(rotIndex+1 == channel-> mNumRotationKeys)
            curRotation = channel->mRotationKeys[rotIndex].mValue;
        else {
            float t0 = channel->mRotationKeys[rotIndex].mTime;   // Interpolate using quaternions
            float t1 = channel->mRotationKeys[rotIndex+1].mTime;
            float weight1 = (poseTime-t0)/(t1-t0); 
 
            aiQuaternion::Interpolate(curRotation, channel->mRotationKeys[rotIndex].mValue, 
                                      channel->mRotationKeys[rotIndex+1].mValue, weight1);
            curRotation = curRotation.Normalize();
        }
             
	// Now allow all the matrix interpolations to be done in assimp's row order convention
        aiMatrix4x4 trafo = aiMatrix4x4(curRotation.GetMatrix());             // now build a rotation matrix
        trafo.a4 = curPosition.x; trafo.b4 = curPosition.y; trafo.c4 = curPosition.z; // add the translation
        targetNode->mTransformation = trafo;  // assign this transformation to the node
    }

    // Calculate the total transformation for each bone relative to the rest pose
    for(unsigned int a=0; a<mesh->mNumBones; a++) { 
        const aiBone* bone = mesh->mBones[a];
        aiMatrix4x4 bTrans = bone->mOffsetMatrix;  // start with mesh-to-bone matrix to subtract rest pose

        // Find the bone, then loop through the nodes/bones on the path up to the root. 
        for(aiNode* node = scene->mRootNode->FindNode(bone->mName); node!=NULL; node=node->mParent)
            bTrans = node->mTransformation * bTrans;   // add each bone's current relative transformation
        
		/* original version below used the row order matrices of angel.h
		boneTransforms[a] =  mat4(	vec4(bTrans.a1, bTrans.a2, bTrans.a3, bTrans.a4),
									vec4(bTrans.b1, bTrans.b2, bTrans.b3, bTrans.b4),
									vec4(bTrans.c1, bTrans.c2, bTrans.c3, bTrans.c4),
									vec4(bTrans.d1, bTrans.d2, bTrans.d3, bTrans.d4));   // Convert to mat4
		*/

		/*	We transpose manually for GLM's column order matrices (and it works!)
			Its simplest to just transpose the final result rather than to transpose each of the scene's matrices and 
			all of the intermediate matrices
		*/
        boneTransforms[a] =  glm::mat4(	glm::vec4(bTrans.a1, bTrans.b1, bTrans.c1, bTrans.d1),
										glm::vec4(bTrans.a2, bTrans.b2, bTrans.c2, bTrans.d2),
										glm::vec4(bTrans.a3, bTrans.b3, bTrans.c3, bTrans.d3), 
										glm::vec4(bTrans.a4, bTrans.b4, bTrans.c4, bTrans.d4));
    }
}
