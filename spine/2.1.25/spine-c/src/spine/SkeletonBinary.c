/******************************************************************************
 * Spine Runtimes Software License
 * Version 2.3
 *
 * Copyright (c) 2013-2015, Esoteric Software
 * All rights reserved.
 *
 * You are granted a perpetual, non-exclusive, non-sublicensable and
 * non-transferable license to use, install, execute and perform the Spine
 * Runtimes Software (the "Software") and derivative works solely for personal
 * or internal use. Without the written permission of Esoteric Software (see
 * Section 2 of the Spine Software License Agreement), you may not (a) modify,
 * translate, adapt or otherwise create derivative works, improvements of the
 * Software or develop new applications using the Software or (b) remove,
 * delete, alter or obscure any trademarks or any copyright, trademark, patent
 * or other intellectual property or proprietary rights notices on or in the
 * Software, including any copy thereof. Redistributions in binary or source
 * form must include this license and terms.
 *
 * THIS SOFTWARE IS PROVIDED BY ESOTERIC SOFTWARE "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL ESOTERIC SOFTWARE BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#include <assert.h>
#include <spine/SkeletonBinary.h>
#include <stdio.h>
#include <spine/extension.h>
#include <spine/AtlasAttachmentLoader.h>
#include <spine/Animation.h>
#include "kvec.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

typedef struct {
	const unsigned char* cursor;
	const unsigned char* end;
} _dataInput;

typedef struct {
	const char* parent;
	const char* skin;
	int slotIndex;
	spMeshAttachment* mesh;
} _spLinkedMesh;

typedef struct {
	spSkeletonBinary super;
	int ownsLoader;

	int linkedMeshCount;
	int linkedMeshCapacity;
	_spLinkedMesh* linkedMeshes;
} _spSkeletonBinary;

spSkeletonBinary* spSkeletonBinary_createWithLoader(spAttachmentLoader* attachmentLoader) {
	spSkeletonBinary* self = SUPER(NEW(_spSkeletonBinary));
	self->scale = 1;
	self->attachmentLoader = attachmentLoader;
	return self;
}

spSkeletonBinary* spSkeletonBinary_create(spAtlas* atlas) {
	spAtlasAttachmentLoader* attachmentLoader = spAtlasAttachmentLoader_create(atlas);
	spSkeletonBinary* self = spSkeletonBinary_createWithLoader(SUPER(attachmentLoader));
	SUB_CAST(_spSkeletonBinary, self)->ownsLoader = 1;
	return self;
}

void spSkeletonBinary_dispose(spSkeletonBinary* self) {
	_spSkeletonBinary* internal = SUB_CAST(_spSkeletonBinary, self);
	if (internal->ownsLoader) spAttachmentLoader_dispose(self->attachmentLoader);
	FREE(internal->linkedMeshes);
	FREE(self->error);
	FREE(self);
}

void _spSkeletonBinary_setError(spSkeletonBinary* self, const char* value1, const char* value2) {
	char message[256];
	int length;
	FREE(self->error);
	strcpy(message, value1);
	length = (int)strlen(value1);
	if (value2) strncat(message + length, value2, 255 - length);
	MALLOC_STR(self->error, message);
}

static unsigned char readByte(_dataInput* input) {
	return *input->cursor++;
}

static char readSByte(_dataInput* input) {
	return (char)readByte(input);
}

static int readBoolean(_dataInput* input) {
	return readByte(input) != 0;
}

static int readInt(_dataInput* input) {
	int result = readByte(input);
	result <<= 8;
	result |= readByte(input);
	result <<= 8;
	result |= readByte(input);
	result <<= 8;
	result |= readByte(input);
	return result;
}

static int readVarint(_dataInput* input, int/*bool*/optimizePositive) {
	unsigned char b = readByte(input);
	int value = b & 0x7F;
	if (b & 0x80) {
		b = readByte(input);
		value |= (b & 0x7F) << 7;
		if (b & 0x80) {
			b = readByte(input);
			value |= (b & 0x7F) << 14;
			if (b & 0x80) {
				b = readByte(input);
				value |= (b & 0x7F) << 21;
				if (b & 0x80) value |= (readByte(input) & 0x7F) << 28;
			}
		}
	}
	if (!optimizePositive) value = (((unsigned int)value >> 1) ^ -(value & 1));
	return value;
}

float readFloat(_dataInput* input) {
	union {
		int intValue;
		float floatValue;
	} intToFloat;
	intToFloat.intValue = readInt(input);
	return intToFloat.floatValue;
}

char* readString(_dataInput* input) {
	int length = readVarint(input, 1);
	char* string;
	if (length == 0) {
		return 0;
	}
	string = MALLOC(char, length);
	memcpy(string, input->cursor, length - 1);
	input->cursor += length - 1;
	string[length - 1] = '\0';
	return string;
}

static void readColor(_dataInput* input, float* r, float* g, float* b, float* a) {
	*r = readByte(input) / 255.0f;
	*g = readByte(input) / 255.0f;
	*b = readByte(input) / 255.0f;
	*a = readByte(input) / 255.0f;
}

#define ATTACHMENT_REGION 0
#define ATTACHMENT_BOUNDING_BOX 1
#define ATTACHMENT_MESH 2
#define ATTACHMENT_LINKED_MESH 3
#define ATTACHMENT_PATH 4

#define BLEND_MODE_NORMAL 0
#define BLEND_MODE_ADDITIVE 1
#define BLEND_MODE_MULTIPLY 2
#define BLEND_MODE_SCREEN 3

#define CURVE_LINEAR 0
#define CURVE_STEPPED 1
#define CURVE_BEZIER 2

#define BONE_ROTATE 0
#define BONE_TRANSLATE 1
#define BONE_SCALE 2
#define BONE_SHEAR 3

#define SLOT_ATTACHMENT 0
#define SLOT_COLOR 1

#define PATH_POSITION 0
#define PATH_SPACING 1
#define PATH_MIX 2

#define PATH_POSITION_FIXED 0
#define PATH_POSITION_PERCENT 1

#define PATH_SPACING_LENGTH 0
#define PATH_SPACING_FIXED 1
#define PATH_SPACING_PERCENT 2

#define PATH_ROTATE_TANGENT 0
#define PATH_ROTATE_CHAIN 1
#define PATH_ROTATE_CHAIN_SCALE 2

#define TIMELINE_SCALE 0
#define TIMELINE_ROTATE 1
#define TIMELINE_TRANSLATE 2
#define TIMELINE_ATTACHMENT 3
#define TIMELINE_COLOR 4
#define TIMELINE_FLIPX 5
#define TIMELINE_FLIPY 6

static void readCurve(_dataInput* input, spCurveTimeline* timeline, int frameIndex) {
	switch (readByte(input)) {
	case CURVE_STEPPED: {
		spCurveTimeline_setStepped(timeline, frameIndex);
		break;
	}
	case CURVE_BEZIER: {
		float cx1 = readFloat(input);
		float cy1 = readFloat(input);
		float cx2 = readFloat(input);
		float cy2 = readFloat(input);
		spCurveTimeline_setCurve(timeline, frameIndex, cx1, cy1, cx2, cy2);
		break;
	}
	}
}

static spAnimation* _spSkeletonBinary_readAnimation(spSkeletonBinary* self, const char* name,
	_dataInput* input, spSkeletonData* skeletonData) {
	kvec_t(spTimeline*) timelines;
	float duration = 0;
	int i, n, ii, nn;
	int frameIndex;
	int drawOrderCount, eventCount;
	spAnimation* animation;

	kv_init(timelines);

	/* Slot timelines. */
	for (i = 0, n = readVarint(input, 1); i < n; ++i) {
		int slotIndex = readVarint(input, 1);
		for (ii = 0, nn = readVarint(input, 1); ii < nn; ++ii) {
			unsigned char timelineType = readByte(input);
			int frameCount = readVarint(input, 1);
			switch (timelineType) {
			case TIMELINE_COLOR: {
				spColorTimeline* timeline = spColorTimeline_create(frameCount);
				timeline->slotIndex = slotIndex;
				for (frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
					float time = readFloat(input);
					float r, g, b, a;
					readColor(input, &r, &g, &b, &a);
					spColorTimeline_setFrame(timeline, frameIndex, time, r, g, b, a);
					if (frameIndex < frameCount - 1) readCurve(input, SUPER(timeline), frameIndex);
				}
				kv_push(spTimeline*, timelines, SUPER(SUPER(timeline)));
				duration = MAX(duration, timeline->frames[(frameCount - 1) * 5]);
				break;
			}
			case TIMELINE_ATTACHMENT: {
				spAttachmentTimeline* timeline = spAttachmentTimeline_create(frameCount);
				timeline->slotIndex = slotIndex;
				for (frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
					float time = readFloat(input);
					const char* attachmentName = readString(input);
					/* TODO Avoid copying of attachmentName inside */
					spAttachmentTimeline_setFrame(timeline, frameIndex, time, attachmentName);
					FREE(attachmentName);
				}
				kv_push(spTimeline*, timelines, SUPER(timeline));
				duration = MAX(duration, timeline->frames[frameCount - 1]);
				break;
			}
			default: {
				int i;
				for (i = 0; i < kv_size(timelines); ++i)
					spTimeline_dispose(kv_A(timelines, i));
				kv_destroy(timelines);
				_spSkeletonBinary_setError(self, "Invalid timeline type for a slot: ", skeletonData->slots[slotIndex]->name);
				return 0;
			}
			}
		}
	}

	/* Bone timelines. */
	for (i = 0, n = readVarint(input, 1); i < n; ++i) {
		int boneIndex = readVarint(input, 1);
		for (ii = 0, nn = readVarint(input, 1); ii < nn; ++ii) {
			unsigned char timelineType = readByte(input);
			int frameCount = readVarint(input, 1);
			switch (timelineType) {
			case TIMELINE_ROTATE: {
				spRotateTimeline* timeline = spRotateTimeline_create(frameCount);
				timeline->boneIndex = boneIndex;
				for (frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
					float time = readFloat(input);
					float degrees = readFloat(input);
					spRotateTimeline_setFrame(timeline, frameIndex, time, degrees);
					if (frameIndex < frameCount - 1) readCurve(input, SUPER(timeline), frameIndex);
				}
				kv_push(spTimeline*, timelines, SUPER(SUPER(timeline)));
				duration = MAX(duration, timeline->frames[(frameCount - 1) * 2]);
				break;
			}
			case TIMELINE_TRANSLATE:
			case TIMELINE_SCALE: {
				float timelineScale = 1;
				spTranslateTimeline* timeline = 0;
				switch (timelineType) {
				case TIMELINE_SCALE:
					timeline = spScaleTimeline_create(frameCount);
					break;
				default:
					timeline = spTranslateTimeline_create(frameCount);
					timelineScale = self->scale;
					break;
				}
				timeline->boneIndex = boneIndex;
				for (frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
					float time = readFloat(input);
					float x = readFloat(input) * timelineScale;
					float y = readFloat(input) * timelineScale;
					spTranslateTimeline_setFrame(timeline, frameIndex, time, x, y);
					if (frameIndex < frameCount - 1) readCurve(input, SUPER(timeline), frameIndex);
				}
				kv_push(spTimeline*, timelines, SUPER_CAST(spTimeline, timeline));
				duration = MAX(duration, timeline->frames[(frameCount - 1) * 3]);
				break;
			}
			case TIMELINE_FLIPX:
			case TIMELINE_FLIPY: {
				spFlipTimeline* timeline = spFlipTimeline_create(frameCount, timelineType == TIMELINE_FLIPX);
				timeline->boneIndex = boneIndex;
				for (frameIndex = 0; frameIndex < frameCount; frameIndex++) {
					float time = readFloat(input);
					int flip = readBoolean(input);
					spFlipTimeline_setFrame(timeline, frameIndex, time, flip);
				}
				kv_push(spTimeline*, timelines, SUPER_CAST(spTimeline, timeline));
				duration = MAX(duration, timeline->frames[(frameCount - 1) * 2]);
				break;
			}
			default: {
				int i;
				for (i = 0; i < kv_size(timelines); ++i)
					spTimeline_dispose(kv_A(timelines, i));
				kv_destroy(timelines);
				_spSkeletonBinary_setError(self, "Invalid timeline type for a bone: ", skeletonData->bones[boneIndex]->name);
				return 0;
			}
			}
		}
	}

	/* IK constraint timelines. */
	for (i = 0, n = readVarint(input, 1); i < n; ++i) {
		int index = readVarint(input, 1);
		int frameCount = readVarint(input, 1);
		spIkConstraintTimeline* timeline = spIkConstraintTimeline_create(frameCount);
		timeline->ikConstraintIndex = index;
		for (frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
			float time = readFloat(input);
			float mix = readFloat(input);
			char bendDirection = readSByte(input);
			spIkConstraintTimeline_setFrame(timeline, frameIndex, time, mix, bendDirection);
			if (frameIndex < frameCount - 1) readCurve(input, SUPER(timeline), frameIndex);
		}
		kv_push(spTimeline*, timelines, SUPER(SUPER(timeline)));
		duration = MAX(duration, timeline->frames[(frameCount - 1) * 3]);
	}

	for (i = 0, n = readVarint(input, 1); i < n; i++) {
		int index = readVarint(input, 1);
		spSkin* skin = skeletonData->skins[index];
		for (int ii = 0, nn = readVarint(input, 1); ii < nn; ii++) {
			int slotIndex = readVarint(input, 1);
			for (int iii = 0, nnn = readVarint(input, 1); iii < nnn; iii++) {
				spAttachment* attachment = spSkin_getAttachment(skin, slotIndex, readString(input));
				int frameCount = readVarint(input, 1);

				int vertexCount = 0;
				if (attachment->type == SP_ATTACHMENT_MESH)
					vertexCount = SUB_CAST(spMeshAttachment, attachment)->verticesCount << 1; // TODO: x2?
				else
					vertexCount = SUB_CAST(spSkinnedMeshAttachment, attachment)->weightsCount / 3 * 2;

				spFFDTimeline* timeline = spFFDTimeline_create(frameCount, vertexCount);
				timeline->slotIndex = slotIndex;
				timeline->attachment = attachment;

				for (int frameIndex = 0; frameIndex < frameCount; frameIndex++) {
					float time = readFloat(input);

					float* vertices = NULL;

					int end = readVarint(input, 1);
					if (end == 0) {
						if (attachment->type == SP_ATTACHMENT_MESH)
							vertices = SUB_CAST(spMeshAttachment, attachment)->vertices;
						else
							vertices = MALLOC(float, vertexCount);
					}
					else {
						vertices = MALLOC(float, vertexCount);
						int start = readVarint(input, 1);
						end += start;
						if (self->scale == 1) {
							for (int v = start; v < end; v++)
								vertices[v] = readFloat(input);
						}
						else {
							for (int v = start; v < end; v++)
								vertices[v] = readFloat(input) * self->scale;
						}
						if (attachment->type ==  SP_ATTACHMENT_MESH) {
							float* meshVertices = SUB_CAST(spMeshAttachment, attachment)->vertices;
							for (int v = 0, vn = SUB_CAST(spMeshAttachment, attachment)->verticesCount; v < vn; v++)
								vertices[v] += meshVertices[v];
						}
					}

					spFFDTimeline_setFrame(timeline, i, time, vertices);
					if (frameIndex < frameCount - 1) readCurve(input, SUPER(timeline), frameIndex);
				}

				kv_push(spTimeline*, timelines, SUPER(SUPER(timeline)));
				duration = MAX(duration, timeline->frames[frameCount - 1]);
			}
		}
	}

	/* Draw order timeline. */
	drawOrderCount = readVarint(input, 1);
	if (drawOrderCount) {
		spDrawOrderTimeline* timeline = spDrawOrderTimeline_create(drawOrderCount, skeletonData->slotsCount);
		for (i = 0; i < drawOrderCount; ++i) {
			int offsetCount = readVarint(input, 1);
			int* drawOrder = MALLOC(int, skeletonData->slotsCount);
			int* unchanged = MALLOC(int, skeletonData->slotsCount - offsetCount);
			int originalIndex = 0, unchangedIndex = 0;
			memset(drawOrder, -1, sizeof(int) * skeletonData->slotsCount);
			for (ii = 0; ii < offsetCount; ++ii) {
				int slotIndex = readVarint(input, 1);
				/* Collect unchanged items. */
				while (originalIndex != slotIndex)
					unchanged[unchangedIndex++] = originalIndex++;
				/* Set changed items. */
				drawOrder[originalIndex + readVarint(input, 1)] = originalIndex;
				++originalIndex;
			}
			/* Collect remaining unchanged items. */
			while (originalIndex < skeletonData->slotsCount)
				unchanged[unchangedIndex++] = originalIndex++;
			/* Fill in unchanged items. */
			for (ii = skeletonData->slotsCount - 1; ii >= 0; ii--)
				if (drawOrder[ii] == -1) drawOrder[ii] = unchanged[--unchangedIndex];
			FREE(unchanged);
			/* TODO Avoid copying of drawOrder inside */
			float time = readFloat(input); // TODO: ???
			spDrawOrderTimeline_setFrame(timeline, i, time, drawOrder);
			FREE(drawOrder);
		}
		kv_push(spTimeline*, timelines, SUPER(timeline));
		duration = MAX(duration, timeline->frames[drawOrderCount - 1]);
	}

	/* Event timeline. */
	eventCount = readVarint(input, 1);
	if (eventCount) {
		spEventTimeline* timeline = spEventTimeline_create(eventCount);
		for (i = 0; i < eventCount; ++i) {
			float time = readFloat(input);
			spEventData* eventData = skeletonData->events[readVarint(input, 1)];
			spEvent* event = spEvent_create(eventData);
			event->intValue = readVarint(input, 0);
			event->floatValue = readFloat(input);
			if (readBoolean(input))
				event->stringValue = readString(input);
			else
				MALLOC_STR(event->stringValue, eventData->stringValue);
			spEventTimeline_setFrame(timeline, i, time, event);
		}
		kv_push(spTimeline*, timelines, SUPER(timeline));
		duration = MAX(duration, timeline->frames[eventCount - 1]);
	}

	kv_trim(spTimeline*, timelines);

	animation = spAnimation_create(name, 0);
	animation->duration = duration;
	animation->timelinesCount = kv_size(timelines);
	animation->timelines = kv_array(timelines);
	return animation;
}

static float* _readFloatArray(_dataInput* input, int* pn, float scale) {
	int n = readVarint(input, 1);
	float* arr = MALLOC(float, n);
	int i;
	if (scale == 1)
		for (i = 0; i < n; ++i)
			arr[i] = readFloat(input);
	else
		for (i = 0; i < n; ++i)
			arr[i] = readFloat(input) * scale;
	*pn = n;
	return arr;
}

static short* _readShortArray(_dataInput* input, int* length) {
	int n = readVarint(input, 1);
	short* array = MALLOC(short, n);
	int i;
	*length = n;
	for (i = 0; i < n; ++i) {
		array[i] = readByte(input) << 8;
		array[i] |= readByte(input);
	}
	return array;
}

static int* _readIntArray(_dataInput* input, int* length) {
	int n = readVarint(input, 1);
	int* array = MALLOC(int, n);
	int i;
	*length = n;
	for (i = 0; i < n; ++i) {
		array[i] = readVarint(input, 1);
	}
	return array;
}

typedef struct arrayHelper_t {
	size_t elementSize;
	int* countPtr;
	void** dataPtr;
	void* header;
	void* end;
	void* current;
} arrayHelper;

arrayHelper* createArrayHelper(void** header, size_t elementSize, int ensure, int* countPtr) {
	arrayHelper* r = MALLOC(arrayHelper, 1);
	r->elementSize = elementSize;
	r->countPtr = countPtr;
	r->dataPtr = header;
	r->header = _malloc(elementSize * ensure, __FILE__, __LINE__);
	r->end = ((char*)r->header) + elementSize * ensure;
	r->current = r->header;
	*header = r->header;
	return r;
}

void disposeArrayHelper(arrayHelper* r) {
	*r->dataPtr = realloc(*r->dataPtr, ((char*)r->end) - ((char*)r->header));
	FREE(r);
}

void appendArrayElement(arrayHelper* r, void* data) {
	if (r->current == r->end) {
		ptrdiff_t s = (char*)r->end - (char*)r->header;
		r->header = *r->dataPtr = realloc(*r->dataPtr, s << 1);
		r->current = (char*)r->header + s;
		r->end = (char*)r->header + (s << 1);
	}
	memcpy(r->current, data, r->elementSize);
	(char*)r->current += r->elementSize;
	*r->countPtr += 1;
}

spAttachment* spSkeletonBinary_readAttachment(spSkeletonBinary* self, _dataInput* input,
	spSkin* skin, int slotIndex, const char* attachmentName, int/*bool*/ nonessential) {
	int i;
	spAttachmentType type;
	const char* name = readString(input);
	if (!name) MALLOC_STR(name, attachmentName);

	type = (spAttachmentType)readByte(input);

	switch (type) {
	case SP_ATTACHMENT_REGION: {
		const char* path = readString(input);
		if (!path) MALLOC_STR(path, name);
		spAttachment* attachment = spAttachmentLoader_newAttachment(self->attachmentLoader, skin, type, attachmentName, path);
		spRegionAttachment* region = SUB_CAST(spRegionAttachment, attachment);
		region->path = path;
		region->x = readFloat(input) * self->scale;
		region->y = readFloat(input) * self->scale;
		region->scaleX = readFloat(input);
		region->scaleY = readFloat(input);
		region->rotation = readFloat(input);
		region->width = readFloat(input) * self->scale;
		region->height = readFloat(input) * self->scale;
		readColor(input, &region->r, &region->g, &region->b, &region->a);
		spRegionAttachment_updateOffset(region);
		return attachment;
	}
	case SP_ATTACHMENT_BOUNDING_BOX: {
		spAttachment* attachment = spAttachmentLoader_newAttachment(self->attachmentLoader, skin, type, attachmentName, name);
		spBoundingBoxAttachment* box = SUB_CAST(spBoundingBoxAttachment, attachment);
		box->vertices = _readFloatArray(input, &box->verticesCount, self->scale);
		return SUPER(box);
	}
	case SP_ATTACHMENT_MESH: {
		const char* path = readString(input);
		if (!path) MALLOC_STR(path, name);
		spAttachment* attachment = spAttachmentLoader_newAttachment(self->attachmentLoader, skin, type, attachmentName, path);
		int vertexCount;
		spMeshAttachment* mesh = SUB_CAST(spMeshAttachment, attachment);
		mesh->path = path;

		mesh->regionUVs = _readFloatArray(input, &vertexCount, 1);
		unsigned short* tr = _readShortArray(input, &mesh->trianglesCount);
		mesh->triangles = MALLOC(int, mesh->trianglesCount);
		for (int i = 0; i < mesh->trianglesCount; ++i) {
			mesh->triangles[i] = tr[i];
		}
		FREE(tr);
		mesh->vertices = _readFloatArray(input, &mesh->verticesCount, self->scale);
		spMeshAttachment_updateUVs(mesh);

		readColor(input, &mesh->r, &mesh->g, &mesh->b, &mesh->a);
		mesh->hullLength = readVarint(input, 1) << 1;
		if (nonessential) {
			mesh->edges = (int*)_readIntArray(input, &mesh->edgesCount);
			mesh->width = readFloat(input) * self->scale;
			mesh->height = readFloat(input) * self->scale;
		}
		else {
			mesh->edges = 0;
			mesh->width = 0;
			mesh->height = 0;
		}
		return SUPER(mesh);
	}
	case 3: {
		const char* path;
		path = readString(input);
		if (!path) MALLOC_STR(path, name);
		spAttachment* attachment = spAttachmentLoader_newAttachment(self->attachmentLoader, skin, type, attachmentName, path);
		spSkinnedMeshAttachment* mesh = SUB_CAST(spSkinnedMeshAttachment, attachment);
		mesh->path = path;
		mesh->regionUVs = _readFloatArray(input, &mesh->uvsCount, 1);
		unsigned short* tr = _readShortArray(input, &mesh->trianglesCount);
		mesh->triangles = MALLOC(int, mesh->trianglesCount);
		for (int i = 0; i < mesh->trianglesCount; ++i) {
			mesh->triangles[i] = tr[i];
		}
		FREE(tr);
		int vertexCount = readVarint(input, 1);
		arrayHelper* weights = createArrayHelper(&mesh->weights, sizeof(mesh->weights[0]), mesh->uvsCount * 9, &mesh->weightsCount);
		arrayHelper* bones = createArrayHelper(&mesh->bones, sizeof(mesh->bones[0]), mesh->uvsCount * 3, &mesh->bonesCount);
		for (i = 0; i < vertexCount; ++i) {
			int bonesCount = (int)readFloat(input);
			appendArrayElement(bones, &bonesCount);
			for (int nn = i + bonesCount * 4; i < nn; i += 4) {
				bonesCount = (int)readFloat(input);
				appendArrayElement(bones, &bonesCount);
				assert(mesh->bones[mesh->bonesCount - 1] == bonesCount);
				float w;
				w = readFloat(input) * self->scale;
				appendArrayElement(weights, &w);
				w = readFloat(input) * self->scale;
				appendArrayElement(weights, &w);
				w = readFloat(input);
				appendArrayElement(weights, &w);
			}
		}
		disposeArrayHelper(weights);
		disposeArrayHelper(bones);
		weights = NULL;
		bones = NULL;

		spSkinnedMeshAttachment_updateUVs(mesh);
		readColor(input, &mesh->r, &mesh->g, &mesh->b, &mesh->a);
		mesh->hullLength = readVarint(input, 1) * 2;
		if (nonessential) {
			mesh->edges = _readIntArray(input, &mesh->edgesCount);
			mesh->width = readFloat(input) * self->scale;
			mesh->height = readFloat(input) * self->scale;
		}
		return SUPER(mesh);
	}
	default:
		assert(0);
	}

	return 0;
}

spSkin* spSkeletonBinary_readSkin(spSkeletonBinary* self, _dataInput* input,
	const char* skinName, int/*bool*/ nonessential) {
	spSkin* skin;
	int slotCount = readVarint(input, 1);
	int i, ii, nn;
	if (slotCount == 0)
		return 0;
	skin = spSkin_create(skinName);
	for (i = 0; i < slotCount; ++i) {
		int slotIndex = readVarint(input, 1);
		for (ii = 0, nn = readVarint(input, 1); ii < nn; ++ii) {
			const char* name = readString(input);
			spSkin_addAttachment(skin, slotIndex, name,
				spSkeletonBinary_readAttachment(self, input, skin, slotIndex, name, nonessential));
			FREE(name);
		}
	}
	return skin;
}

spSkeletonData* spSkeletonBinary_readSkeletonDataFile(spSkeletonBinary* self, const char* path) {
	int length;
	spSkeletonData* skeletonData;
	const char* binary = _spUtil_readFile(path, &length);
	if (length == 0 || !binary) {
		_spSkeletonBinary_setError(self, "Unable to read skeleton file: ", path);
		return 0;
	}
	skeletonData = spSkeletonBinary_readSkeletonData(self, (unsigned char*)binary, length);
	FREE(binary);
	return skeletonData;
}

spSkeletonData* spSkeletonBinary_readSkeletonData(spSkeletonBinary* self, const unsigned char* binary,
	const int length) {
	int i, ii, nonessential;
	spSkeletonData* skeletonData;
	_spSkeletonBinary* internal = SUB_CAST(_spSkeletonBinary, self);

	_dataInput* input = NEW(_dataInput);
	input->cursor = binary;
	input->end = binary + length;

	FREE(self->error);
	CONST_CAST(char*, self->error) = 0;
	internal->linkedMeshCount = 0;

	skeletonData = spSkeletonData_create();

	skeletonData->hash = readString(input);
	if (!strlen(skeletonData->hash)) {
		FREE(skeletonData->hash);
		skeletonData->hash = 0;
	}

	skeletonData->version = readString(input);
	if (!strlen(skeletonData->version)) {
		FREE(skeletonData->version);
		skeletonData->version = 0;
	}

	skeletonData->width = readFloat(input);
	skeletonData->height = readFloat(input);

	nonessential = readBoolean(input);

	if (nonessential) FREE(readString(input)); /* Skip images path. */

	/* Bones. */
	skeletonData->bonesCount = readVarint(input, 1);
	skeletonData->bones = MALLOC(spBoneData*, skeletonData->bonesCount);
	for (i = 0; i < skeletonData->bonesCount; ++i) {
		spBoneData* data;
		const char* name = readString(input);
		int parentIndex = readVarint(input, 1);
		spBoneData* parent = parentIndex == 0 ? 0 : skeletonData->bones[parentIndex - 1];
		/* TODO Avoid copying of name */
		data = spBoneData_create(name, parent);
		FREE(name);
		data->x = readFloat(input) * self->scale;
		data->y = readFloat(input) * self->scale;
		data->scaleX = readFloat(input);
		data->scaleY = readFloat(input);
		data->rotation = readFloat(input);
		data->length = readFloat(input) * self->scale;
		data->flipX = readBoolean(input);
		data->flipY = readBoolean(input);
		data->inheritScale = readBoolean(input);
		data->inheritRotation = readBoolean(input);
		if (nonessential) readInt(input); /* Skip bone color. */
		skeletonData->bones[i] = data;
	}

	/* IK constraints. */
	skeletonData->ikConstraintsCount = readVarint(input, 1);
	skeletonData->ikConstraints = MALLOC(spIkConstraintData*, skeletonData->ikConstraintsCount);
	for (i = 0; i < skeletonData->ikConstraintsCount; ++i) {
		const char* name = readString(input);
		/* TODO Avoid copying of name */
		spIkConstraintData* data = spIkConstraintData_create(name);
		FREE(name);
		data->bonesCount = readVarint(input, 1);
		data->bones = MALLOC(spBoneData*, data->bonesCount);
		for (ii = 0; ii < data->bonesCount; ++ii)
			data->bones[ii] = skeletonData->bones[readVarint(input, 1)];
		data->target = skeletonData->bones[readVarint(input, 1)];
		data->mix = readFloat(input);
		data->bendDirection = readSByte(input);
		skeletonData->ikConstraints[i] = data;
	}

	/* Slots. */
	skeletonData->slotsCount = readVarint(input, 1);
	skeletonData->slots = MALLOC(spSlotData*, skeletonData->slotsCount);
	for (i = 0; i < skeletonData->slotsCount; ++i) {
		const char* slotName = readString(input);
		spBoneData* boneData = skeletonData->bones[readVarint(input, 1)];
		/* TODO Avoid copying of slotName */
		spSlotData* slotData = spSlotData_create(slotName, boneData);
		FREE(slotName);
		readColor(input, &slotData->r, &slotData->g, &slotData->b, &slotData->a);
		slotData->attachmentName = readString(input);
		slotData->additiveBlending = readBoolean(input);
		skeletonData->slots[i] = slotData;
	}

	/* Default skin. */
	skeletonData->defaultSkin = spSkeletonBinary_readSkin(self, input, "default", nonessential);
	skeletonData->skinsCount = readVarint(input, 1);

	if (skeletonData->defaultSkin)
		++skeletonData->skinsCount;

	skeletonData->skins = MALLOC(spSkin*, skeletonData->skinsCount);

	if (skeletonData->defaultSkin)
		skeletonData->skins[0] = skeletonData->defaultSkin;

	/* Skins. */
	for (i = skeletonData->defaultSkin ? 1 : 0; i < skeletonData->skinsCount; ++i) {
		const char* skinName = readString(input);
		/* TODO Avoid copying of skinName */
		skeletonData->skins[i] = spSkeletonBinary_readSkin(self, input, skinName, nonessential);
		FREE(skinName);
	}

	/* Events. */
	skeletonData->eventsCount = readVarint(input, 1);
	skeletonData->events = MALLOC(spEventData*, skeletonData->eventsCount);
	for (i = 0; i < skeletonData->eventsCount; ++i) {
		const char* name = readString(input);
		/* TODO Avoid copying of skinName */
		spEventData* eventData = spEventData_create(name);
		FREE(name);
		eventData->intValue = readVarint(input, 0);
		eventData->floatValue = readFloat(input);
		eventData->stringValue = readString(input);
		skeletonData->events[i] = eventData;
	}

	/* Animations. */
	skeletonData->animationsCount = readVarint(input, 1);
	skeletonData->animations = MALLOC(spAnimation*, skeletonData->animationsCount);
	for (i = 0; i < skeletonData->animationsCount; ++i) {
		spAnimation* animation = _spSkeletonBinary_readAnimation(self, readString(input), input, skeletonData);
		if (!animation) {
			FREE(input);
			spSkeletonData_dispose(skeletonData);
			return 0;
		}
		skeletonData->animations[i] = animation;
	}

	FREE(input);
	return skeletonData;
}
