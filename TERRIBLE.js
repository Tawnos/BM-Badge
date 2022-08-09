var ENGINE_VERSION = 3; // MUST BE BUMPED BY WHOLE NUMBERS WHEN ENGINE CHANGES DATA SHAPES!!!
var IS_LITTLE_ENDIAN = true;
var IS_SCREEN_LITTLE_ENDIAN = false;
var IS_GLITCHED_FLAG = 0b10000000;
var IS_DEBUG_FLAG = 0b01000000;
var IS_FLIPPED_DIAGONAL_FLAG = 0b00000001;
var MAX_ENTITIES_PER_MAP = 64;
var DIALOG_SCREEN_NO_PORTRAIT = 255;
var DIALOG_SCREEN_NO_ENTITY = 255;

var getFileJson = function (file) {
	return file.text()
		.then(function (text) {
			return JSON.parse(text);
		});
};

var combineArrayBuffers = function (bufferA, bufferB) {
	var temp = new Uint8Array(bufferA.byteLength + bufferB.byteLength);
	temp.set(new Uint8Array(bufferA), 0);
	temp.set(new Uint8Array(bufferB), bufferA.byteLength);
	return temp.buffer;
};

var setCharsIntoDataView = function (
	dataView,
	string,
	offset,
	maxLength
) {
	var source = string.slice(0, maxLength);
	for (var i = 0; i < source.length; i++) {
		dataView.setUint8(
			i + offset,
			source.charCodeAt(i)
		);
	}
};

var getPaddedHeaderLength = function (length) {
	// pad to to uint32_t alignment
	var mod = length % 4;
	return length + (
		mod
			? 4 - mod
			: 0
	);
};

var propertyTypeHandlerMap = {
	'object': function (value, targetSourceList) {
		// tiled object ids always start at 1
		return value === 0
			? null
			: targetSourceList.find(function (item) {
				return item.id === value;
			});
	},
	'string': function (value, targetSourceList) {
		return value;
	},
	'bool': function (value, targetSourceList) {
		return value === true;
	},
	'color': function (value, targetSourceList) {
		var chunks = value
			.replace('#', '')
			.match(/.{1,2}/g)
			.map(function (chunk) {
				return parseInt(chunk, 10);
			});
		return rgbaToC565(
			chunks[1],
			chunks[2],
			chunks[3],
			chunks[0]
		);
	},
	'file': function (value, targetSourceList) {
		return value;
	},
	'float': function (value, targetSourceList) {
		return parseFloat(value);
	},
	'int': function (value, targetSourceList) {
		return parseInt(value, 10);
	}
};

var mergeInProperties = function (target, properties, targetSourceList) {
	var list = targetSourceList || [];
	if (properties) {
		properties.forEach(function (property) {
			target[property.name] = propertyTypeHandlerMap[property.type](
				property.value,
				list
			);
		});
	}
	return target;
};

var assignToLessFalsy = function () {
	var inputArray = Array.prototype.slice.call(arguments);
	var target = inputArray.shift();
	inputArray.forEach(function (source) {
		Object.keys(source).forEach(function (key) {
			var value = source[key];
			if (
				value === ""
				|| value === undefined
				|| value === null
			) {
				value = target[key];
			}
			if (
				value === ""
				|| value === undefined
				|| value === null
			) {
				value = null;
			}
			target[key] = value;
		});
	});
	return target;
};

var jsonClone = function (input) {
	return JSON.parse(JSON.stringify(input));
};

var makeComputedStoreGetterSetter = function (propertyName) {
	return {
		get: function () {
			return this.$store.state[propertyName]
		},
		set: function (value) {
			return this.$store.commit('GENERIC_MUTATOR', {
				propertyName: propertyName,
				value: value,
			});
		}
	}
};

var makeComputedStoreGetterSettersMixin = function (config) {
	var vuexPropertyNames = config;
	var computedNames = config;
	var computed = {};
	if (!(config instanceof Array)) {
		vuexPropertyNames = Object.values(config);
		computedNames = Object.keys(config);
	}
	vuexPropertyNames.forEach(function (vuexPropertyName, index) {
		computed[computedNames[index]] = makeComputedStoreGetterSetter(
			vuexPropertyName
		);
	});
	return {
		computed: computed
	};
}

var makeFileChangeTrackerMixinByResourceType = function (resourceName) {
	var fileMapPropertyName = resourceName + 'FileItemMap';
	var resourceJsonStatePropertyName = resourceName + 'JsonOutput'
	var changedFileMapPropertyName = resourceName + 'ChangedFileMap'
	return {
		computed: {
			startFileMap: function () {
				return this.getAllRecombinedFilesForResource(this.initState, resourceName);
			},
			[changedFileMapPropertyName]: function () {
				var result = {}
				var currentFileMap = this.getAllRecombinedFilesForResource(this.currentData, resourceName)
				var startFileMap = this.startFileMap
				Object.keys(currentFileMap).forEach(function (fileName) {
					if (currentFileMap[fileName] !== startFileMap[fileName]) {
						result[fileName] = currentFileMap[fileName]
					}
				})
				return result;
			},
			[resourceJsonStatePropertyName]: function () {
				return JSON.stringify(this.currentData[fileMapPropertyName]);
			},
			[resourceName + 'NeedSave']: function () {
				return Object.keys(this[changedFileMapPropertyName]).length > 0;
			},
		},
		methods: {
			getAllRecombinedFilesForResource (source, resourceName) {
				var fileMapPropertyName = resourceName + 'FileItemMap';
				var self = this;
				var result = {};
				Object.keys(
					source[fileMapPropertyName]
				).forEach(function (fileName) {
					result[fileName] = self.recombineFileForResource(
						fileName,
						source
					);
				});
				return result
			},
			recombineFileForResource (fileName, source) {
				var resourceNamesInFile = source[fileMapPropertyName][fileName];
				var result = {};
				resourceNamesInFile.forEach(function (scriptName) {
					result[scriptName] = source[resourceName][scriptName];
				});
				return JSON.stringify(result, null, '\t') + '\n';
			},
		}
	}
}// reference: https://doc.mapeditor.org/en/stable/reference/tmx-map-format/#tile-flipping
var FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
var FLIPPED_VERTICALLY_FLAG   = 0x40000000;
var FLIPPED_DIAGONALLY_FLAG   = 0x20000000;
var getMapTileAndOrientationByGID = function (tileGID, map) {
	var targetTileset = {};
	var tileId = tileGID;
	// Clear the flags
	tileId &= ~(
		FLIPPED_HORIZONTALLY_FLAG
		| FLIPPED_VERTICALLY_FLAG
		| FLIPPED_DIAGONALLY_FLAG
	);
	map.tilesets.find(function (tileset, index) {
		var overshot = tileId < tileset.firstgid;
		if(!overshot) {
			targetTileset = tileset;
		}
		return overshot;
	});
	var tileIndex = tileId - targetTileset.firstgid;

	var flip_x = !!(tileGID & FLIPPED_HORIZONTALLY_FLAG);
	var flip_y = !!(tileGID & FLIPPED_VERTICALLY_FLAG);
	var flip_diag = !!(tileGID & FLIPPED_DIAGONALLY_FLAG);
	return {
		tileset: targetTileset,
		tileIndex: tileIndex,
		flip_x: flip_x,
		flip_y: flip_y,
		flip_diag: flip_diag,
		renderFlags: (
			(flip_x << 2)
			+ (flip_y << 1)
			+ (flip_diag << 0)
		),
		tile: (targetTileset.parsed.tiles || []).find(function (tile) {
			return tile.id === tileIndex;
		}) || {
			id: tileIndex
		}
	}
};

var handleTileLayer = function(layer, map) {
	var bytesPerTile = 4;
	var mapLayerByteSize = bytesPerTile * map.height * map.width;
	var serializedLayer = new ArrayBuffer(mapLayerByteSize);
	var dataView = new DataView(serializedLayer);
	var offset = 0;
	var tileGid;
	var tileData;
	while (offset < layer.data.length) {
		tileGid = layer.data[offset];
		if (tileGid) { // if tileGid is 0, it's an empty tile, no work to do
			tileData = getMapTileAndOrientationByGID(tileGid, map);
			dataView.setUint16(
				(offset * bytesPerTile),
				tileData.tileIndex + 1, // because 0 must mean "empty"
				IS_LITTLE_ENDIAN
			);
			dataView.setUint8(
				(offset * bytesPerTile) + 2,
				tileData.tileset.parsed.scenarioIndex
			);
			dataView.setUint8(
				(offset * bytesPerTile) + 3,
				tileData.renderFlags
			);
		}
		offset += 1;
	}
	map.serializedLayers.push(serializedLayer);
};

var compositeEntityInheritedData = function (entity, map, objects, fileNameMap, scenarioData) {
	entity.sourceMap = map.name;
	var tileData = getMapTileAndOrientationByGID(
		entity.gid,
		map
	);
	entity.flip_x = tileData.flip_x;
	entity.flip_y = tileData.flip_y;
	entity.flip_diag = tileData.flip_diag;
	mergeInProperties(
		entity,
		entity.properties,
		objects
	);
	var mergedWithTile = assignToLessFalsy(
		{},
		tileData.tile,
		entity
	);
	var entityPrototype = (
		scenarioData.entityTypesPlusProperties[mergedWithTile.type]
		|| scenarioData.entityTypes[mergedWithTile.type]
	);
	var compositeEntity = assignToLessFalsy(
		{},
		entityPrototype || {},
		mergedWithTile
	);
	compositeEntity.renderFlags = tileData.renderFlags;
	compositeEntity.tileIndex = tileData.tileIndex;
	compositeEntity.tileset = tileData.tileset
	// console.table([
	//  entityPrototype,
	//  entity.tile,
	//  entity,
	//  mergedWithType,
	//  compositeEntity
	// ])
	entity.compositeEntity = compositeEntity;
};

var handleTiledObjectAsEntity = function (entity, map, objects, fileNameMap, scenarioData) {
	serializeEntity(
		entity.compositeEntity,
		fileNameMap,
		scenarioData,
	);
	entity.mapIndex = map.entityIndices.length;
	map.entityIndices.push(
		entity.compositeEntity.scenarioIndex
	);
	if (entity.compositeEntity.is_player) {
		if (map.playerEntityId !== specialKeywordsEnum['%MAP%']) {
			var entityALabel = entity.compositeEntity.name || entity.compositeEntity.type;
			var entityB = map.entityObjects[map.playerEntityId];
			var entityBLabel = entityB.compositeEntity.name || entityB.compositeEntity.type;
			throw new Error(`More than one entity on map "${map.name}" has \`is_player\` checked, this is not allowed!\nCompeting entities: "${entityALabel}", "${entityBLabel}"`);
		} else {
			map.playerEntityId = entity.mapIndex;
		}
	}
}

var handleMapTilesets = function (mapTilesets, scenarioData, fileNameMap) {
	return Promise.all(mapTilesets.map(function (mapTilesetItem) {
		return loadTilesetByName(
			mapTilesetItem.source,
			fileNameMap,
			scenarioData,
		).then(function (tilesetParsed) {
			mapTilesetItem.parsed = tilesetParsed;
		});
	}));
};

var handleMapLayers = function (map, scenarioData, fileNameMap) {
	map.layers.filter(function (layer) {
		return layer.type === 'tilelayer';
	}).forEach(function (tileLayer) {
		handleTileLayer(tileLayer, map);
	})
	var allObjectsOnAllObjectLayers = [];
	map.layers.filter(function (layer) {
		return layer.type === 'objectgroup';
	}).forEach(function (objectLayer) {
		allObjectsOnAllObjectLayers = allObjectsOnAllObjectLayers
			.concat(objectLayer.objects);
	});
	allObjectsOnAllObjectLayers.forEach(function (tiledObject) {
		if (tiledObject.rotation) {
			throw new Error(`The Encoder WILL NOT SUPPORT object rotation! Go un-rotate and encode again! Object was found on map: ${
				map.name
			};\nOffending object was: ${
				JSON.stringify(tiledObject, null, '\t')
			}`);
		}
	});
	map.entityObjects = allObjectsOnAllObjectLayers.filter(function(object) {
		return object.gid !== undefined;
	});
	if (map.entityObjects.length > MAX_ENTITIES_PER_MAP) {
		throw new Error(
			`Map "${
				map.name
			}" has ${
				map.entityObjects.length
			} entities, but the limit is ${
				MAX_ENTITIES_PER_MAP
			}`
		);
	}
	map.geometryObjects = allObjectsOnAllObjectLayers.filter(function(object) {
		return object.gid === undefined;
	});
	map.geometryObjects.forEach(function (tiledObject) {
		handleTiledObjectAsGeometry(
			tiledObject,
			fileNameMap,
			scenarioData,
			map,
		);
	});
	map.playerEntityId = specialKeywordsEnum['%MAP%'];
	map.entityObjects.forEach(function (tiledObject) {
		compositeEntityInheritedData(
			tiledObject,
			map,
			map.geometryObjects,
			fileNameMap,
			scenarioData,
		);
	});
	map.entityObjects.sort(function (a, b) {
		return a.compositeEntity.is_debug === b.compositeEntity.is_debug
			? 0 // same value, don't move
			: !a.compositeEntity.is_debug
				? -1 // b is debug, move a left
				: 1; // a is debug, move a right
	});
	map.entityObjects.forEach(function (tiledObject) {
		handleTiledObjectAsEntity(
			tiledObject,
			map,
			map.geometryObjects,
			fileNameMap,
			scenarioData,
		);
	});
	return map;
};

var generateMapHeader = function (map) {
	var goDirections = map.directions || {};
	var goDirectionCount = Object.keys(goDirections).length;
	var headerLength = (
		16 // char[] name
		+ 2 // uint16_t tile_width
		+ 2 // uint16_t tile_height
		+ 2 // uint16_t cols
		+ 2 // uint16_t rows
		+ 2 // uint16_t on_load
		+ 2 // uint16_t on_tick
		+ 2 // uint16_t on_look
		+ 1 // uint8_t layer_count
		+ 1 // uint8_t player_entity_id
		+ 2 // uint16_t entity_count
		+ 2 // uint16_t geometry_count
		+ 2 // uint16_t script_count
		+ 1 // uint16_t go_direction_count
		+ 1 // uint16_t count_padding
		+ (
			2 // uint16_t entity_id
			* map.entityIndices.length
		)
		+ (
			2 // uint16_t geometry_id
			* map.geometryIndices.length
		)
		+ (
			2 // uint16_t script_id
			* map.scriptIndices.length
		)
		+ (
			16 // go_direction custom type
			* goDirectionCount
		)
	);
	var result = new ArrayBuffer(
		getPaddedHeaderLength(headerLength)
	);
	var dataView = new DataView(result);
	var offset = 0;
	setCharsIntoDataView(
		dataView,
		map.name,
		0,
		offset += 16
	);
	dataView.setUint16(
		offset,
		map.tilewidth,
		IS_LITTLE_ENDIAN
	);
	offset += 2;
	dataView.setUint16(
		offset,
		map.tileheight,
		IS_LITTLE_ENDIAN
	);
	offset += 2;
	dataView.setUint16(
		offset,
		map.width,
		IS_LITTLE_ENDIAN
	);
	offset += 2;
	dataView.setUint16(
		offset,
		map.height,
		IS_LITTLE_ENDIAN
	);
	offset += 2;
	dataView.setUint16(
		offset,
		map.on_load || 0,
		IS_LITTLE_ENDIAN
	);
	offset += 2;
	dataView.setUint16(
		offset,
		map.on_tick || 0,
		IS_LITTLE_ENDIAN
	);
	offset += 2;
	dataView.setUint16(
		offset,
		map.on_look || 0,
		IS_LITTLE_ENDIAN
	);
	offset += 2;
	dataView.setUint8(
		offset,
		map.serializedLayers.length
	);
	offset += 1;
	dataView.setUint8(
		offset,
		map.playerEntityId
	);
	offset += 1;
	dataView.setUint16(
		offset,
		map.entityIndices.length,
		IS_LITTLE_ENDIAN
	);
	offset += 2;
	dataView.setUint16(
		offset,
		map.geometryIndices.length,
		IS_LITTLE_ENDIAN
	);
	offset += 2;
	dataView.setUint16(
		offset,
		map.scriptIndices.length,
		IS_LITTLE_ENDIAN
	);
	offset += 2;
	dataView.setUint8(
		offset,
		goDirectionCount,
		IS_LITTLE_ENDIAN
	);
	offset += 1;
	offset += 1; // u1 padding
	map.entityIndices.forEach(function (entityIndex) {
		dataView.setUint16(
			offset,
			entityIndex,
			IS_LITTLE_ENDIAN
		);
		offset += 2;
	});
	map.geometryIndices.forEach(function (geometryIndex) {
		dataView.setUint16(
			offset,
			geometryIndex,
			IS_LITTLE_ENDIAN
		);
		offset += 2;
	});
	map.scriptIndices.forEach(function (scriptIndex) {
		dataView.setUint16(
			offset,
			scriptIndex,
			IS_LITTLE_ENDIAN
		);
		offset += 2;
	});
	Object.keys(goDirections).forEach(function (directionName) {
		setCharsIntoDataView(
			dataView,
			directionName,
			offset,
			offset += 12,
		);
		dataView.setUint16(
			offset,
			map.directionScriptIds[directionName],
			IS_LITTLE_ENDIAN
		);
		offset += 2;
		offset += 2; // padding to get back to 16 byte alignment
	});
	return result;
};

var handleMapData = function (
	name,
	mapFile,
	mapProperties,
	fileNameMap,
	scenarioData,
) {
	return function (map) {
		// console.log(
		// 	'Map:',
		// 	mapFile.name,
		// 	map
		// );
		map.name = name;
		Object.assign(
			map,
			(mapProperties || {}),
		);
		mapFile.parsed = map;
		map.scenarioIndex = mapFile.scenarioIndex;
		map.entityIndices = [];
		map.geometryIndices = [];
		map.scriptIndices = [];
		map.scriptNameKeys = {};
		map.serializedLayers = [];
		scenarioData.parsed.maps[mapFile.scenarioIndex] = map;
		return handleMapTilesets(map.tilesets, scenarioData, fileNameMap)
			.then(function () {
				handleMapLayers(map, scenarioData, fileNameMap);
				handleMapScripts(
					map,
					fileNameMap,
					scenarioData,
				);
				map.serialized = generateMapHeader(map);
				map.serializedLayers.forEach(function (layer) {
					map.serialized = combineArrayBuffers(
						map.serialized,
						layer
					);
				})
				return map;
			});
	};
};

var mergeMapDataIntoScenario = function(
	fileNameMap,
	scenarioData,
) {
	var result = Promise.resolve();
	var mapsFile = fileNameMap['maps.json'];
	if (mapsFile) {
		result = getFileJson(mapsFile).then(function (mapsData) {
			if(scenarioData.maps && mapsData) {
				Object.keys(mapsData).forEach(function (key) {
					if(scenarioData.maps[key]) {
						throw new Error(`Map "${key}" has duplicate definition in both "scenario.json" and "maps.json"! Remove one to continue!`);
					}
				});
			}
			scenarioData.maps = Object.assign(
				{},
				scenarioData.maps || {},
				mapsData
			);
		});
	}
	return result;
};

var handleScenarioMaps = function (scenarioData, fileNameMap) {
	var maps = scenarioData.maps;
	var orderedMapPromise = Promise.resolve();
	Object.keys(maps).forEach(function (key) {
		var mapProperties = maps[key];
		if (typeof mapProperties === 'string') {
			mapProperties = {
				path: mapProperties
			};
		}
		if (!mapProperties.path) {
			throw new Error(`Map "${key}" is missing a "path" property, cannot load map!`);
		}
		var mapFileName = mapProperties.path.split('/').pop();
		var mapFile = fileNameMap[mapFileName];
		mapFile.scenarioIndex = scenarioData.parsed.maps.length;
		scenarioData.parsed.maps.push({
			name: key,
			scenarioIndex: mapFile.scenarioIndex
		});
		scenarioData.mapsByName[key] = mapFile;
		if (!mapFile) {
			throw new Error(
				'Map `' + mapFileName + '` could not be found in folder!'
			);
		} else {
			orderedMapPromise = orderedMapPromise.then(function() {
				return getFileJson(mapFile)
					.then(handleMapData(
						key,
						mapFile,
						mapProperties,
						fileNameMap,
						scenarioData,
					));
			});
		}
	});
	return orderedMapPromise;
};var serializeTileset = function (tilesetData, image) {
	var tilesetHeaderLength = getPaddedHeaderLength(
		16 // char[16] name
		+ 2 // uint16_t imageIndex
		+ 2 // uint16_t imageWidth
		+ 2 // uint16_t imageHeight
		+ 2 // uint16_t tileWidth
		+ 2 // uint16_t tileHeight
		+ 2 // uint16_t cols
		+ 2 // uint16_t rows
	);
	var header = new ArrayBuffer(
		tilesetHeaderLength
	);
	var dataView = new DataView(header);
	var offset = 0;
	setCharsIntoDataView(
		dataView,
		tilesetData.name,
		0,
		offset += 16
	);
	dataView.setUint16(
		offset, // uint16_t imageIndex
		image.scenarioIndex,
		IS_LITTLE_ENDIAN
	);
	offset += 2
	dataView.setUint16(
		offset, // uint16_t imageWidth
		tilesetData.tilewidth, // used to be tilesetData.imagewidth,
		IS_LITTLE_ENDIAN
	);
	offset += 2
	dataView.setUint16(
		offset, // uint16_t imageHeight
		tilesetData.rows * tilesetData.columns * tilesetData.tileheight, // used to be tilesetData.imageheight
		IS_LITTLE_ENDIAN
	);
	offset += 2
	dataView.setUint16(
		offset, // uint16_t tileWidth
		tilesetData.tilewidth,
		IS_LITTLE_ENDIAN
	);
	offset += 2
	dataView.setUint16(
		offset, // uint16_t tileHeight
		tilesetData.tileheight,
		IS_LITTLE_ENDIAN
	);
	offset += 2
	dataView.setUint16(
		offset, // uint16_t cols
		1, // used to be tilesetData.columns,
		IS_LITTLE_ENDIAN
	);
	offset += 2
	dataView.setUint16(
		offset, // uint16_t rows
		tilesetData.rows * tilesetData.columns,
		IS_LITTLE_ENDIAN
	);
	var result = combineArrayBuffers(
		header,
		tilesetData.serializedTiles
	);
	return result;
};

var handleTilesetData = function (tilesetFile, scenarioData, fileNameMap) {
	return function (tilesetData) {
		tilesetData.filename = tilesetFile.name;
		tilesetData.scenarioIndex = tilesetFile.scenarioIndex;
		scenarioData.parsed.tilesets[tilesetData.scenarioIndex] = tilesetData;
		// console.log(
		// 	'Tileset:',
		// 	tilesetFile.name,
		// 	tilesetData
		// );
		scenarioData.tilesetMap[tilesetData.filename] = tilesetData;
		tilesetData.serializedTiles = new ArrayBuffer(
			getPaddedHeaderLength(tilesetData.tilecount * 2)
		);
		var tileDataView = new DataView(tilesetData.serializedTiles);
		// forget about the built-in name, using file name instead.
		tilesetData.name = tilesetFile.name.split('.')[0];
		// already has columns, add the missing pair
		tilesetData.rows = Math.floor(tilesetData.imageheight / tilesetData.tileheight);
		(tilesetData.tiles || []).forEach(function (tile) {
			mergeInProperties(
				tile,
				tile.properties
			);
			var entityPrototype = (
				scenarioData.entityTypesPlusProperties[tile.type]
				|| {}
			);
			Object.assign(
				tile,
				assignToLessFalsy(
					{},
					entityPrototype,
					tile
				)
			);
			if (
				tile.objectgroup
				&& tile.objectgroup.objects
			) {
				// we probably have tile geometry!
				if (tile.objectgroup.objects.length > 1) {
					throw new Error(`${tilesetData.name} has more than one geometry on a single tile!`);
				}
				var geometry = handleTiledObjectAsGeometry(
					tile.objectgroup.objects[0],
					fileNameMap,
					scenarioData,
				);
				if (geometry) {
					tileDataView.setUint16(
						tile.id * 2,
						geometry.scenarioIndex + 1, // because if it's 0, we shouldn't have to
						IS_LITTLE_ENDIAN,
					);
				}
			}
			if (tile.animation) {
				serializeAnimationData(tile, tilesetData, scenarioData);
			}
		});
		var filePromise = handleImage(tilesetData, scenarioData, fileNameMap)
			.then(function () {
				return tilesetData;
			});
		var imageFileName = tilesetData.image.split('/').pop();
		tilesetData.imageFileName = imageFileName;
		var imageFile = fileNameMap[imageFileName];
		tilesetData.imageFile = imageFile;
		tilesetData.serialized = serializeTileset(tilesetData, imageFile);
		tilesetFile.parsed = tilesetData;
		return filePromise
	};
};

var loadTilesetByName = function(
	tilesetFileName,
	fileNameMap,
	scenarioData,
) {
	var tilesetFileNameSplit = tilesetFileName.split('/').pop();
	var tilesetFile = fileNameMap[tilesetFileNameSplit];
	if (!tilesetFile) {
		throw new Error(
			'Tileset `' + tilesetFileNameSplit + '` could not be found in folder!'
		);
	} else {
		if (tilesetFile.scenarioIndex === undefined) {
			tilesetFile.scenarioIndex = scenarioData.parsed.tilesets.length;
			scenarioData.parsed.tilesets.push({
				name: `temporary - ${tilesetFileNameSplit} - awaiting parse`,
				scenarioIndex: tilesetFile.scenarioIndex
			});
		}
		return (
			tilesetFile.parsed
				? Promise.resolve(tilesetFile.parsed)
				: getFileJson(tilesetFile)
					.then(handleTilesetData(tilesetFile, scenarioData, fileNameMap))
		)
	}
};

var getPreloadedTilesetByName = function(
	tilesetFileName,
	fileNameMap,
	scenarioData,
) {
	var tilesetFileNameSplit = tilesetFileName.split('/').pop();
	var tilesetFile = fileNameMap[tilesetFileNameSplit];
	if (!tilesetFile.parsed) {
		throw new Error(
			'Tileset `' + tilesetFileNameSplit + '` was not loaded at the time it was requested!'
		);
	}
	return tilesetFile.parsed;
};
var serializeAnimationData = function (tile, tilesetData, scenarioData) {
	tile.animation.scenarioIndex = scenarioData.parsed.animations.length;
	scenarioData.parsed.animations.push(tile.animation);
	var headerLength = (
		2 // uint16_t tileset_id
		+ 2 // uint16_t frame_count
		+ (
			(
				2 // uint16_t tileid
				+ 2 // uint16_t duration
			)
			* tile.animation.length
		)
	);
	tile.animation.serialized = new ArrayBuffer(
		getPaddedHeaderLength(headerLength)
	);
	var dataView = new DataView(tile.animation.serialized);
	var offset = 0;
	dataView.setUint16(
		offset,
		tilesetData.scenarioIndex,
		IS_LITTLE_ENDIAN
	);
	offset += 2;
	dataView.setUint16(
		offset,
		tile.animation.length,
		IS_LITTLE_ENDIAN
	);
	offset += 2;
	tile.animation.forEach(function (frame) {
		dataView.setUint16(
			offset,
			frame.tileid,
			IS_LITTLE_ENDIAN
		);
		offset += 2;
		dataView.setUint16(
			offset,
			frame.duration,
			IS_LITTLE_ENDIAN,
		);
		offset += 2;
	});
};
var handleEntityTypesData = function (
	fileNameMap,
	scenarioData
) {
	return function (entityTypesData) {
		scenarioData.entityTypes = entityTypesData;
		var objectTypesFile = fileNameMap['object_types.json'];
		return (!objectTypesFile
			? Promise.resolve([])
			: getFileJson(objectTypesFile)
		)
			.then(handleObjectTypesData(
				fileNameMap,
				scenarioData,
			));
	};
};

var defaultEntityTypeProperties = [
	{
		"name": "hackable_state_a",
		"type": "int",
		"value": 0
	},
	{
		"name": "hackable_state_b",
		"type": "int",
		"value": 0
	},
	{
		"name": "hackable_state_c",
		"type": "int",
		"value": 0
	},
	{
		"name": "hackable_state_d",
		"type": "int",
		"value": 0
	},
	{
		"name": "is_glitched",
		"type": "bool",
		"value": false
	},
	{
		"name": "is_player",
		"type": "bool",
		"value": false
	},
	{
		"name": "on_interact",
		"type": "string",
		"value": ""
	},
	{
		"name": "on_tick",
		"type": "string",
		"value": ""
	},
	{
		"name": "path",
		"type": "object",
		"value": 0
	}
];

var handleObjectTypesData = function (
	fileNameMap,
	scenarioData,
) {
	return function (objectTypesData) {
		// console.log(
		// 	'object_types.json',
		// 	objectTypesData
		// );
		var entityTilesetsPromiseArray = Object.keys(scenarioData.entityTypes).map(function (key) {
			var entityType = scenarioData.entityTypes[key];
			entityType.type = key;
			entityType.scenarioIndex = scenarioData.parsed.entityTypes.length;
			scenarioData.parsed.entityTypes.push(entityType);
			var entityTypePlusProperties = jsonClone(entityType);
			var objectProperties = objectTypesData.find(function (objectTypeEntity) {
				return objectTypeEntity.name === key;
			}) || defaultEntityTypeProperties.slice();
			if (objectProperties) {
				mergeInProperties(
					entityTypePlusProperties,
					objectProperties.properties
				);
				scenarioData.entityTypesPlusProperties[key] = entityTypePlusProperties;
			}
			return loadTilesetByName(
				entityType.tileset,
				fileNameMap,
				scenarioData,
			)
				.then(function () {
					entityType.serialized = serializeEntityType(
						entityTypePlusProperties,
						fileNameMap,
						scenarioData,
					);
				});
		});
		return Promise.all(entityTilesetsPromiseArray);
	};
};

var createAnimationDirectionSerializer = function (
	tileset,
	dataView
) {
	return function (direction) {
		var tile = (tileset.parsed.tiles || []).find(function (tile) {
			return tile.id === direction.tileid
		});
		var animation = tile && tile.animation;
		dataView.setUint16(
			dataView.currentOffset, // uint16_t type_id
			animation
				? animation.scenarioIndex
				: direction.tileid,
			IS_LITTLE_ENDIAN
		);
		dataView.currentOffset += 2;
		dataView.setUint8(
			dataView.currentOffset, // uint8_t type
			animation
				? 0
				: tileset.parsed.scenarioIndex + 1
		);
		dataView.currentOffset += 1;
		dataView.setUint8(
			dataView.currentOffset, // uint8_t render_flags
			(
				(direction.flip_x << 2)
				+ (direction.flip_y << 1)
				+ (direction.flip_diag << 0)
			)
		);
		dataView.currentOffset += 1;
	};
};
var animationDirectionSize = (
	+ 2 // uint16_t type_id
	+ 1 // uint8_t type (
	// 0: type_id is the ID of an animation,
	// !0: type is now a lookup on the tileset table,
	// and type_id is the ID of the tile on that tileset
	// )
	+ 1 // uint8_t render_flags
);
var serializeEntityType = function (
	entityType,
	fileNameMap,
	scenarioData,
) {
	var portraitKey = entityType.portrait || entityType.type;
	var portrait = scenarioData.portraits[portraitKey];
	var portraitIndex = portrait
		? portrait.scenarioIndex
		: DIALOG_SCREEN_NO_PORTRAIT;
	var animations = Object.values(entityType.animations);
	var headerLength = (
		32 // char[32] name
		+ 1 // uint8_t ??? padding
		+ 1 // uint8_t ??? padding
		+ 1 // uint8_t portrait_index
		+ 1 // uint8_t animation_count
		+ (
			animationDirectionSize
			* 4 // the number of directions supported in the engine
			* animations.length
		)
	);
	var result = new ArrayBuffer(
		getPaddedHeaderLength(headerLength)
	);
	var dataView = new DataView(result);
	dataView.currentOffset = 0;
	setCharsIntoDataView(
		dataView,
		entityType.name || entityType.type,
		0,
		dataView.currentOffset += 32
	);
	dataView.currentOffset += 2; // padding
	dataView.setUint8(
		dataView.currentOffset, // uint8_t animation_count
		portraitIndex
	);
	dataView.currentOffset += 1;
	dataView.setUint8(
		dataView.currentOffset, // uint8_t animation_count
		animations.length
	);
	dataView.currentOffset += 1;
	var tileset = fileNameMap[entityType.tileset];
	var serializeAnimation = createAnimationDirectionSerializer(
		tileset,
		dataView,
	);
	animations.forEach(function (animation) {
		animation.forEach(serializeAnimation);
	});
	return result;
};
var handlePortraitsData = function (
	fileNameMap,
	scenarioData,
) {
	return function (portraitsData) {
		scenarioData.portraits = portraitsData;
		var portraitTilesetsPromiseArray = Object.keys(scenarioData.portraits).map(function (key) {
			var portrait = scenarioData.portraits[key];
			portrait.scenarioIndex = scenarioData.parsed.portraits.length;
			scenarioData.parsed.portraits.push(portrait);
			return loadTilesetByName(
				portrait.tileset,
				fileNameMap,
				scenarioData,
			)
				.then(function () {
					portrait.serialized = serializePortrait(
						key,
						portrait,
						fileNameMap
					);
				});
		});
		return Promise.all(portraitTilesetsPromiseArray);
	}
};

var serializePortrait = function(
	portraitName,
	portrait,
	fileNameMap
) {
	var emotes = Object.values(portrait.emotes);
	var tileset = fileNameMap[portrait.tileset];
	var headerLength = (
		32 // char[32] name
		+ 1 // uint8_t ??? padding
		+ 1 // uint8_t ??? padding
		+ 1 // uint8_t ??? padding
		+ 1 // uint8_t emote_count
		+ (
			animationDirectionSize
			* emotes.length
		)
	);
	var result = new ArrayBuffer(
		getPaddedHeaderLength(headerLength)
	);
	var dataView = new DataView(result);
	dataView.currentOffset = 0;
	setCharsIntoDataView(
		dataView,
		portraitName,
		0,
		dataView.currentOffset += 32
	);
	dataView.currentOffset += 3; // padding
	dataView.setUint8(
		dataView.currentOffset, // uint8_t emote_count
		emotes.length
	);
	dataView.currentOffset += 1;
	var serializeAnimation = createAnimationDirectionSerializer(
		tileset,
		dataView,
	);
	emotes.forEach(serializeAnimation);
	return result;
};
var serializeEntity = function (
	entity,
	fileNameMap,
	scenarioData,
) {
	var headerLength = (
		12 // char[12] name
		+ 2 // uint16_t x
		+ 2 // uint16_t y
		+ 2 // uint16_t on_interact_script_id // local index to the map's script list
		+ 2 // uint16_t on_tick_script_id // local index to the map's script list
		+ 2 // uint16_t primary_id // may be: entity_type_id, animation_id, tileset_id
		+ 2 // uint16_t secondary_id // if primary_id_type is tileset_id, this is the tile_id, otherwise 0
		+ 1 // uint8_t primary_id_type
		+ 1 // uint8_t current_animation
		+ 1 // uint8_t current_frame
		+ 1 // uint8_t direction OR render_flags
		+ 1 // uint8_t hackable_state_a
		+ 1 // uint8_t hackable_state_b
		+ 1 // uint8_t hackable_state_c
		+ 1 // uint8_t hackable_state_d
	);
	var arrayBuffer = new ArrayBuffer(
		getPaddedHeaderLength(headerLength)
	);
	var dataView = new DataView(arrayBuffer);
	var offset = 0;
	setCharsIntoDataView(
		dataView,
		entity.name || entity.type || '',
		0,
		offset += 12
	);
	dataView.setUint16(
		offset, // uint16_t x
		Math.round(entity.x),
		IS_LITTLE_ENDIAN
	);
	offset += 2;
	dataView.setUint16(
		offset, // uint16_t y
		Math.round(entity.y),
		IS_LITTLE_ENDIAN
	);
	offset += 2;
	dataView.on_interact_offset = offset;
	dataView.setUint16(
		offset, // uint16_t on_interact_script_id
		0, // set in another loop later
		IS_LITTLE_ENDIAN
	);
	offset += 2;
	dataView.on_tick_offset = offset;
	dataView.setUint16(
		offset, // uint16_t on_tick_script_id
		0, // set in another loop later
		IS_LITTLE_ENDIAN
	);
	offset += 2;
	var entityType = scenarioData.entityTypes[entity.type];
	var primaryIndexType = 0; // tileset_id
	var primaryIndex;
	var secondaryIndex = 0;
	var directionOrRenderFlags = 0;
	if (entityType) {
		primaryIndexType = 2; // entity_type_id
		primaryIndex = entityType.scenarioIndex;
		Object.keys(entityType.animations)
			.find(function (animationName) {
				var animation = entityType.animations[animationName]
				return animation.find(function (animationDirection, direction) {
					if(
						(entity.tileIndex === animationDirection.tileid)
						&& (!!entity.flip_x === !!animationDirection.flip_x)
						&& (!!entity.flip_y === !!animationDirection.flip_y)
					) {
						directionOrRenderFlags = direction;
						return true
					}
				});
			});
	} else if (entity.animation) {
		primaryIndexType = 1; // animation_id
		primaryIndex = entity.animation.scenarioIndex;
	} else {
		primaryIndex = entity.tileset.parsed.scenarioIndex;
		secondaryIndex = entity.tileIndex;
	}
	if(primaryIndexType !== 2) {
		directionOrRenderFlags = entity.renderFlags;
	}
	if(entity.is_glitched) {
		directionOrRenderFlags |= IS_GLITCHED_FLAG;
	}
	if(entity.is_debug) {
		directionOrRenderFlags |= IS_DEBUG_FLAG;
	}
	if(entity.is_flipped_diagonal) {
		directionOrRenderFlags |= IS_FLIPPED_DIAGONAL_FLAG;
	}
	dataView.setUint16(
		offset, // primary_id // may be: entity_type_id, animation_id, tileset_id
		primaryIndex,
		IS_LITTLE_ENDIAN
	);
	offset += 2;
	dataView.setUint16(
		offset, // secondary_id // if primary_id_type is tileset_id, this is the tile_id, otherwise 0
		secondaryIndex,
		IS_LITTLE_ENDIAN
	);
	offset += 2;
	dataView.setUint8(
		offset, // uint8_t primary_id_type
		primaryIndexType
	);
	offset += 1;
	dataView.setUint8(
		offset, // uint8_t currentAnimation
		entity.currentAnimation || 0
	);
	offset += 1;
	dataView.setUint8(
		offset, // uint8_t currentFrame
		entity.current_frame || 0
	);
	offset += 1;
	dataView.setUint8(
		offset, // uint8_t direction OR render_flags
		directionOrRenderFlags
	);
	offset += 1;
	var hackableStateAOffset = offset;
	dataView.setUint8(
		offset, // uint8_t hackable_state_a
		entity.hackable_state_a || 0
	);
	offset += 1;
	dataView.setUint8(
		offset, // uint8_t hackable_state_b
		entity.hackable_state_b || 0
	);
	offset += 1;
	dataView.setUint8(
		offset, // uint8_t hackable_state_c
		entity.hackable_state_c || 0
	);
	offset += 1;
	dataView.setUint8(
		offset, // uint8_t hackable_state_d
		entity.hackable_state_d || 0
	);
	offset += 1;
	if(entity.path) {
		// console.log('This entity has a path!', entity.path);
		dataView.setUint16(
			hackableStateAOffset,
			entity.path.mapIndex,
			IS_LITTLE_ENDIAN
		);
	}
	entity.serialized = arrayBuffer;
	entity.dataView = dataView;
	entity.scenarioIndex = scenarioData.parsed.entities.length;
	scenarioData.parsed.entities.push(entity);
	return entity;
};
var geometryTypeEnum = {
	point: 0,
	polyline: 1,
	polygon: 2,
};

var serializeGeometry = function (
	geometry,
	map,
	fileNameMap,
	scenarioData,
) {
	var segments = geometry.path.length;
	if (geometry.geometryType === 'polyline') {
		segments--;
	} else if (geometry.geometryType === 'point') {
		segments = 0;
	}
	var headerLength = (
		32 // char[32] name
		+ 1 // uint8_t type
		+ 1 // uint8_t point_count
		+ 1 // uint8_t segment_count
		+ 1 // uint8_t padding
		+ 4 // float path_length
		+ (
			geometry.path.length
			* (
				+ 2 // uint16_t x
				+ 2 // uint16_t y
			)
		)
		+ (
			segments
			* 4 // float segment_length
		)
	);
	var arrayBuffer = new ArrayBuffer(
		getPaddedHeaderLength(headerLength)
	);
	var dataView = new DataView(arrayBuffer);
	var offset = 0;
	setCharsIntoDataView(
		dataView,
		geometry.name,
		0,
		offset += 32
	);
	dataView.setUint8(
		offset, // uint8_t type
		geometryTypeEnum[geometry.geometryType],
	);
	offset += 1;
	dataView.setUint8(
		offset, // uint8_t point_count
		geometry.path.length,
	);
	offset += 1;
	dataView.setUint8(
		offset, // uint8_t segment_count
		segments,
	);
	offset += 1;
	offset += 1; // uint8_t padding
	var addressOfTotalLength = offset;
	offset += 4; // float total_length
	geometry.path.forEach(function (point) {
		dataView.setUint16(
			offset, // uint16_t x
			Math.round(point.x),
			IS_LITTLE_ENDIAN
		);
		offset += 2;
		dataView.setUint16(
			offset, // uint16_t y
			Math.round(point.y),
			IS_LITTLE_ENDIAN
		);
		offset += 2;
	});
	var totalLength = 0;
	if (geometry.path.length > 1) {
		geometry.path.slice(0, segments).forEach(function (pointA, index) {
			var pointB = geometry.path[(index + 1) % geometry.path.length];
			var diff = {
				x: pointB.x - pointA.x,
				y: pointB.y - pointA.y,
			};
			var segmentLength = Math.sqrt(
				(diff.x * diff.x)
				+ (diff.y * diff.y)
			);
			totalLength += segmentLength;
			dataView.setFloat32(
				offset, // float segment_length
				segmentLength,
				IS_LITTLE_ENDIAN
			);
			offset += 4;
		});
	}
	dataView.setFloat32(
		addressOfTotalLength, // float total_length
		totalLength,
		IS_LITTLE_ENDIAN
	);
	geometry.serialized = arrayBuffer;
	geometry.scenarioIndex = scenarioData.parsed.geometry.length;
	scenarioData.parsed.geometry.push(geometry);
	return geometry;
};

var getGeometryType = function (geometry) {
	var type = 'rect';
	if (geometry.point) {
		type = 'point';
	} else if (geometry.polyline) {
		type = 'polyline';
	} else if (geometry.polygon) {
		type = 'polygon';
	} else if (geometry.ellipse) {
		type = 'ellipse';
	} else if (geometry.text) {
		type = 'text';
	}
	return type;
};

var makeRelativeCoordsAbsolute = function (geometry, points) {
	return points.map(function(point) {
		return {
			x: point.x + geometry.x,
			y: point.y + geometry.y,
		};
	});
};

var geometryTypeHandlerMap = {
	point: function (geometry) {
		return [{
			x: geometry.x,
			y: geometry.y,
		}];
	},
	rect: function (geometry) {
		return [
			{ // top left
				x: geometry.x,
				y: geometry.y,
			},
			{ // top right
				x: geometry.x + geometry.width,
				y: geometry.y,
			},
			{ // bottom right
				x: geometry.x + geometry.width,
				y: geometry.y + geometry.height,
			},
			{ // bottom left
				x: geometry.x,
				y: geometry.y + geometry.height,
			}
		];
	},
	polyline: function (geometry) {
		return makeRelativeCoordsAbsolute(geometry, geometry.polyline)
	},
	polygon: function (geometry) {
		return makeRelativeCoordsAbsolute(geometry, geometry.polygon)
	},
	ellipse: function (geometry) {
		var result = [];
		var points = geometry.points || 16;
		var radFraction = (Math.PI * 2) / points;
		var angle;
		var halfWidth = geometry.width / 2;
		var halfHeight = geometry.height / 2;
		var centerX = geometry.x + halfWidth;
		var centerY = geometry.y + halfHeight;
		for (var i = 0; i < points; i += 1) {
			angle = radFraction * i;
			result.push({
				x: centerX + (Math.cos(angle) * halfWidth),
				y: centerY + (Math.sin(angle) * halfHeight),
			});
		}
		return result;
	},
};

var geometryTypeKeyMap = {
	point: 'point',
	rect: 'polygon',
	polyline: 'polyline',
	polygon: 'polygon',
	ellipse: 'polygon',
};

var getPathFromGeometry = function (geometry) {
	var type = getGeometryType(geometry);
	var handler = geometryTypeHandlerMap[type];
	var result;
	if (handler) {
		result = handler(geometry);
		result.type = type;
		geometry.path = result;
		geometry.geometryType = geometryTypeKeyMap[type];
	} else {
		console.warn(`Unsupported Geometry Type: ${type}\n`);
	}
	return result;
};

var handleTiledObjectAsGeometry = function (
	tiledObject,
	fileNameMap,
	scenarioData,
	map,
) {
	var geometry;
	mergeInProperties(tiledObject, tiledObject.properties);
	var path = getPathFromGeometry(tiledObject);
	if (path) {
		geometry = serializeGeometry(
			tiledObject,
			map,
			fileNameMap,
			scenarioData,
		);
		if (map) {
			geometry.mapIndex = map.geometryIndices.length;
			map.geometryIndices.push(
				geometry.scenarioIndex
			);
		}
	}
	return geometry;
};
var serializeString = function (
	string,
	map,
	fileNameMap,
	scenarioData,
) {
	return serializeSomethingLikeAString(
		string,
		map,
		fileNameMap,
		scenarioData,
		'strings'
	);
};
var serializeSaveFlag = function (
	string,
	map,
	fileNameMap,
	scenarioData,
) {
	return serializeSomethingLikeAString(
		string,
		map,
		fileNameMap,
		scenarioData,
		'save_flags'
	);
};
var serializeVariable = function (
	string,
	map,
	fileNameMap,
	scenarioData,
) {
	var variableId = serializeSomethingLikeAString(
		string,
		map,
		fileNameMap,
		scenarioData,
		'variables'
	);
	if(variableId > 255) {
		throw new Error(`There is a limit of 255 Variables! The one that broke the encoder's back was: "${string}"`);
	}
	return variableId;
};
var serializeSomethingLikeAString = function (
	string,
	map,
	fileNameMap,
	scenarioData,
	destinationPropertyName,
) {
	var parsedString = templatizeString(
		string,
		map,
		scenarioData,
	);
	var scenarioIndex = scenarioData.uniqueStringLikeMaps[destinationPropertyName][parsedString];
	if (scenarioIndex === undefined) {
		// allow for explicit null char at the end
		var paddedLength = getPaddedHeaderLength(parsedString.length + 1);
		var buffer = new ArrayBuffer(paddedLength);
		var dataView = new DataView(buffer);
		setCharsIntoDataView(
			dataView,
			parsedString,
			0,
			paddedLength - 1,
		);
		var encodedString = {
			name: parsedString,
			serialized: buffer,
			scenarioIndex: scenarioData.parsed[destinationPropertyName].length,
		};
		scenarioData.parsed[destinationPropertyName].push(encodedString);
		scenarioIndex = encodedString.scenarioIndex;
		scenarioData.uniqueStringLikeMaps[destinationPropertyName][parsedString] = scenarioIndex;
	}
	return scenarioIndex;
};

var templatizeString = function (
	templateString,
	map,
	scenarioData,
) {
	var entityRegex = /%(.*?)%/gm;
	var variableRegex = /\$(.*?)\$/gm;
	var entityIndexReplaceFunction = function (
		wholeVariable,
		variableName
	) {
		var entityLookupString = specialKeywordsEnum[wholeVariable]
			? wholeVariable
			: variableName;
		var entity = getObjectByNameOnMap(
			entityLookupString,
			map,
			{
				action: templateString
			}
		)
		var entityId = (
			entity.specialIndex
			|| entity.mapIndex
		);
		return `%%${entityId}%%`;
	}
	var variableIndexReplaceFunction = function (
		wholeVariable,
		variableName
	) {
		var variableIndex = scenarioData.uniqueStringLikeMaps.variables[variableName];
		if(variableIndex === undefined) {
			throw new Error(`templatizeString was unable to find the variable "${variableName}" in the string "${templateString}"`);
		}
		return `$$${variableIndex}$$`;
	}
	return templateString
		.replace(
			entityRegex,
			entityIndexReplaceFunction
		)
		.replace(
			variableRegex,
			variableIndexReplaceFunction
		);
};
var actionFieldsMap = {
	NULL_ACTION: null,
	CHECK_ENTITY_NAME: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'string', size: 2},
		{propertyName: 'entity', size: 1},
		{propertyName: 'expected_bool', size: 1},
	],
	CHECK_ENTITY_X: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'expected_u2', size: 2},
		{propertyName: 'entity', size: 1},
		{propertyName: 'expected_bool', size: 1},
	],
	CHECK_ENTITY_Y: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'expected_u2', size: 2},
		{propertyName: 'entity', size: 1},
		{propertyName: 'expected_bool', size: 1},
	],
	CHECK_ENTITY_INTERACT_SCRIPT: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'expected_script', size: 2},
		{propertyName: 'entity', size: 1},
		{propertyName: 'expected_bool', size: 1},
	],
	CHECK_ENTITY_TICK_SCRIPT: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'expected_script', size: 2},
		{propertyName: 'entity', size: 1},
		{propertyName: 'expected_bool', size: 1},
	],
	CHECK_ENTITY_TYPE: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'entity_type', size: 2},
		{propertyName: 'entity', size: 1},
		{propertyName: 'expected_bool', size: 1},
	],
	CHECK_ENTITY_PRIMARY_ID: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'expected_u2', size: 2},
		{propertyName: 'entity', size: 1},
		{propertyName: 'expected_bool', size: 1},
	],
	CHECK_ENTITY_SECONDARY_ID: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'expected_u2', size: 2},
		{propertyName: 'entity', size: 1},
		{propertyName: 'expected_bool', size: 1},
	],
	CHECK_ENTITY_PRIMARY_ID_TYPE: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'entity', size: 1},
		{propertyName: 'expected_byte', size: 1},
		{propertyName: 'expected_bool', size: 1},
	],
	CHECK_ENTITY_CURRENT_ANIMATION: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'entity', size: 1},
		{propertyName: 'expected_byte', size: 1},
		{propertyName: 'expected_bool', size: 1},
	],
	CHECK_ENTITY_CURRENT_FRAME: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'entity', size: 1},
		{propertyName: 'expected_byte', size: 1},
		{propertyName: 'expected_bool', size: 1},
	],
	CHECK_ENTITY_DIRECTION: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'entity', size: 1},
		{propertyName: 'direction', size: 1},
		{propertyName: 'expected_bool', size: 1},
	],
	CHECK_ENTITY_GLITCHED: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'entity', size: 1},
		{propertyName: 'expected_bool', size: 1},
	],
	CHECK_ENTITY_HACKABLE_STATE_A: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'entity', size: 1},
		{propertyName: 'expected_byte', size: 1},
		{propertyName: 'expected_bool', size: 1},
	],
	CHECK_ENTITY_HACKABLE_STATE_B: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'entity', size: 1},
		{propertyName: 'expected_byte', size: 1},
		{propertyName: 'expected_bool', size: 1},
	],
	CHECK_ENTITY_HACKABLE_STATE_C: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'entity', size: 1},
		{propertyName: 'expected_byte', size: 1},
		{propertyName: 'expected_bool', size: 1},
	],
	CHECK_ENTITY_HACKABLE_STATE_D: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'entity', size: 1},
		{propertyName: 'expected_byte', size: 1},
		{propertyName: 'expected_bool', size: 1},
	],
	CHECK_ENTITY_HACKABLE_STATE_A_U2: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'expected_u2', size: 2},
		{propertyName: 'entity', size: 1},
		{propertyName: 'expected_bool', size: 1},
	],
	CHECK_ENTITY_HACKABLE_STATE_C_U2: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'expected_u2', size: 2},
		{propertyName: 'entity', size: 1},
		{propertyName: 'expected_bool', size: 1},
	],
	CHECK_ENTITY_HACKABLE_STATE_A_U4: [
		{propertyName: 'expected_u4', size: 4},
		{propertyName: 'success_script', size: 2},
		{propertyName: 'entity', size: 1},
	],
	CHECK_ENTITY_PATH: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'geometry', size: 2},
		{propertyName: 'entity', size: 1},
		{propertyName: 'expected_bool', size: 1},
	],
	CHECK_SAVE_FLAG: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'save_flag', size: 2},
		{propertyName: 'expected_bool', size: 1},
	],
	CHECK_IF_ENTITY_IS_IN_GEOMETRY: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'geometry', size: 2},
		{propertyName: 'entity', size: 1},
		{propertyName: 'expected_bool', size: 1},
	],
	CHECK_FOR_BUTTON_PRESS: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'button_id', size: 1},
	],
	CHECK_FOR_BUTTON_STATE: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'button_id', size: 1},
		{propertyName: 'expected_bool', size: 1},
	],
	CHECK_WARP_STATE: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'string', size: 2},
		{propertyName: 'expected_bool', size: 1},
	],
	RUN_SCRIPT: [
		{propertyName: 'script', size: 2},
	],
	BLOCKING_DELAY: [
		{propertyName: 'duration', size: 4},
	],
	NON_BLOCKING_DELAY: [
		{propertyName: 'duration', size: 4},
	],
	SET_ENTITY_NAME: [
		{propertyName: 'string', size: 2},
		{propertyName: 'entity', size: 1},
	],
	SET_ENTITY_X: [
		{propertyName: 'u2_value', size: 2},
		{propertyName: 'entity', size: 1},
	],
	SET_ENTITY_Y: [
		{propertyName: 'u2_value', size: 2},
		{propertyName: 'entity', size: 1},
	],
	SET_ENTITY_INTERACT_SCRIPT: [
		{propertyName: 'script', size: 2},
		{propertyName: 'entity', size: 1},
	],
	SET_ENTITY_TICK_SCRIPT: [
		{propertyName: 'script', size: 2},
		{propertyName: 'entity', size: 1},
	],
	SET_ENTITY_TYPE: [
		{propertyName: 'entity_type', size: 2},
		{propertyName: 'entity', size: 1},
	],
	SET_ENTITY_PRIMARY_ID: [
		{propertyName: 'u2_value', size: 2},
		{propertyName: 'entity', size: 1},
	],
	SET_ENTITY_SECONDARY_ID: [
		{propertyName: 'u2_value', size: 2},
		{propertyName: 'entity', size: 1},
	],
	SET_ENTITY_PRIMARY_ID_TYPE: [
		{propertyName: 'byte_value', size: 1},
		{propertyName: 'entity', size: 1},
	],
	SET_ENTITY_CURRENT_ANIMATION: [
		{propertyName: 'byte_value', size: 1},
		{propertyName: 'entity', size: 1},
	],
	SET_ENTITY_CURRENT_FRAME: [
		{propertyName: 'byte_value', size: 1},
		{propertyName: 'entity', size: 1},
	],
	SET_ENTITY_DIRECTION: [
		{propertyName: 'direction', size: 1},
		{propertyName: 'entity', size: 1},
	],
	SET_ENTITY_DIRECTION_RELATIVE: [
		{propertyName: 'relative_direction', size: 1},
		{propertyName: 'entity', size: 1},
	],
	SET_ENTITY_DIRECTION_TARGET_ENTITY: [
		{propertyName: 'target_entity', size: 1},
		{propertyName: 'entity', size: 1},
	],
	SET_ENTITY_DIRECTION_TARGET_GEOMETRY: [
		{propertyName: 'target_geometry', size: 2},
		{propertyName: 'entity', size: 1},
	],
	SET_ENTITY_GLITCHED: [
		{propertyName: 'entity', size: 1},
		{propertyName: 'bool_value', size: 1},
	],
	SET_ENTITY_HACKABLE_STATE_A: [
		{propertyName: 'byte_value', size: 1},
		{propertyName: 'entity', size: 1},
	],
	SET_ENTITY_HACKABLE_STATE_B: [
		{propertyName: 'byte_value', size: 1},
		{propertyName: 'entity', size: 1},
	],
	SET_ENTITY_HACKABLE_STATE_C: [
		{propertyName: 'byte_value', size: 1},
		{propertyName: 'entity', size: 1},
	],
	SET_ENTITY_HACKABLE_STATE_D: [
		{propertyName: 'byte_value', size: 1},
		{propertyName: 'entity', size: 1},
	],
	SET_ENTITY_HACKABLE_STATE_A_U2: [
		{propertyName: 'u2_value', size: 2},
		{propertyName: 'entity', size: 1},
	],
	SET_ENTITY_HACKABLE_STATE_C_U2: [
		{propertyName: 'u2_value', size: 2},
		{propertyName: 'entity', size: 1},
	],
	SET_ENTITY_HACKABLE_STATE_A_U4: [
		{propertyName: 'u4_value', size: 4},
		{propertyName: 'entity', size: 1},
	],
	SET_ENTITY_PATH: [
		{propertyName: 'geometry', size: 2},
		{propertyName: 'entity', size: 1},
	],
	SET_SAVE_FLAG: [
		{propertyName: 'save_flag', size: 2},
		{propertyName: 'bool_value', size: 1},
	],
	SET_PLAYER_CONTROL: [
		{propertyName: 'bool_value', size: 1},
	],
	SET_MAP_TICK_SCRIPT: [
		{propertyName: 'script', size: 2},
	],
	SET_HEX_CURSOR_LOCATION: [
		{propertyName: 'address', size: 2},
	],
	SET_WARP_STATE: [
		{propertyName: 'string', size: 2}
	],
	SET_HEX_EDITOR_STATE: [
		{propertyName: 'bool_value', size: 1},
	],
	SET_HEX_EDITOR_DIALOG_MODE: [
		{propertyName: 'bool_value', size: 1},
	],
	SET_HEX_EDITOR_CONTROL: [
		{propertyName: 'bool_value', size: 1},
	],
	SET_HEX_EDITOR_CONTROL_CLIPBOARD: [
		{propertyName: 'bool_value', size: 1},
	],
	LOAD_MAP: [
		{propertyName: 'map', size: 2},
	],
	SHOW_DIALOG: [
		{propertyName: 'dialog', size: 2},
	],
	PLAY_ENTITY_ANIMATION: [
		{propertyName: 'entity', size: 1},
		{propertyName: 'animation', size: 1},
		{propertyName: 'play_count', size: 1},
	],
	TELEPORT_ENTITY_TO_GEOMETRY: [
		{propertyName: 'geometry', size: 2},
		{propertyName: 'entity', size: 1},
	],
	WALK_ENTITY_TO_GEOMETRY: [
		{propertyName: 'duration', size: 4},
		{propertyName: 'geometry', size: 2},
		{propertyName: 'entity', size: 1},
	],
	WALK_ENTITY_ALONG_GEOMETRY: [
		{propertyName: 'duration', size: 4},
		{propertyName: 'geometry', size: 2},
		{propertyName: 'entity', size: 1},
	],
	LOOP_ENTITY_ALONG_GEOMETRY: [
		{propertyName: 'duration', size: 4},
		{propertyName: 'geometry', size: 2},
		{propertyName: 'entity', size: 1},
	],
	SET_CAMERA_TO_FOLLOW_ENTITY: [
		{propertyName: 'entity', size: 1},
	],
	TELEPORT_CAMERA_TO_GEOMETRY: [
		{propertyName: 'geometry', size: 2},
	],
	PAN_CAMERA_TO_ENTITY: [
		{propertyName: 'duration', size: 4},
		{propertyName: 'entity', size: 1},
	],
	PAN_CAMERA_TO_GEOMETRY: [
		{propertyName: 'duration', size: 4},
		{propertyName: 'geometry', size: 2},
	],
	PAN_CAMERA_ALONG_GEOMETRY: [
		{propertyName: 'duration', size: 4},
		{propertyName: 'geometry', size: 2},
		{propertyName: 'entity', size: 1},
	],
	LOOP_CAMERA_ALONG_GEOMETRY: [
		{propertyName: 'duration', size: 4},
		{propertyName: 'geometry', size: 2},
		{propertyName: 'entity', size: 1},
	],
	SET_SCREEN_SHAKE: [
		{propertyName: 'duration', size: 2},
		{propertyName: 'frequency', size: 2},
		{propertyName: 'amplitude', size: 1},
	],
	SCREEN_FADE_OUT: [
		{propertyName: 'duration', size: 4},
		{propertyName: 'color', size: 2, endian: IS_SCREEN_LITTLE_ENDIAN},
	],
	SCREEN_FADE_IN: [
		{propertyName: 'duration', size: 4},
		{propertyName: 'color', size: 2, endian: IS_SCREEN_LITTLE_ENDIAN},
	],
	MUTATE_VARIABLE: [
		{propertyName: 'value', size: 2},
		{propertyName: 'variable', size: 1},
		{propertyName: 'operation', size: 1},
	],
	MUTATE_VARIABLES: [
		{propertyName: 'variable', size: 1},
		{propertyName: 'source', size: 1},
		{propertyName: 'operation', size: 1},
	],
	COPY_VARIABLE: [
		{propertyName: 'variable', size: 1},
		{propertyName: 'entity', size: 1},
		{propertyName: 'field', size: 1},
		{propertyName: 'inbound', size: 1},
	],
	CHECK_VARIABLE: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'value', size: 2},
		{propertyName: 'variable', size: 1},
		{propertyName: 'comparison', size: 1},
		{propertyName: 'expected_bool', size: 1},
	],
	CHECK_VARIABLES: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'variable', size: 1},
		{propertyName: 'source', size: 1},
		{propertyName: 'comparison', size: 1},
		{propertyName: 'expected_bool', size: 1},
	],
	SLOT_SAVE: [],
	SLOT_LOAD: [
		{propertyName: 'slot', size: 1},
	],
	SLOT_ERASE: [
		{propertyName: 'slot', size: 1},
	],
	SET_CONNECT_SERIAL_DIALOG: [
		{propertyName: 'serial_dialog', size: 2},
	],
	SHOW_SERIAL_DIALOG: [
		{propertyName: 'serial_dialog', size: 2},
	],
	INVENTORY_GET: [
		{propertyName: 'item_name', size: 1},
	],
	INVENTORY_DROP: [
		{propertyName: 'item_name', size: 1},
	],
	CHECK_INVENTORY: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'item_name', size: 1},
		{propertyName: 'expected_bool', size: 1},
	],
	SET_MAP_LOOK_SCRIPT: [
		{propertyName: 'script', size: 2},
	],
	SET_ENTITY_LOOK_SCRIPT: [
		{propertyName: 'script', size: 2},
		{propertyName: 'entity', size: 1},
	],
	SET_TELEPORT_ENABLED: [
		{propertyName: 'bool_value', size: 1},
	],
	CHECK_MAP: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'map', size: 2},
		{propertyName: 'expected_bool', size: 1},
	],
	SET_BLE_FLAG: [
		{propertyName: 'ble_flag', size: 1},
		{propertyName: 'bool_value', size: 1},
	],
	CHECK_BLE_FLAG: [
		{propertyName: 'success_script', size: 2},
		{propertyName: 'ble_flag', size: 1},
		{propertyName: 'expected_bool', size: 1},
	],
};

var actionNames = [
	'NULL_ACTION',
	'CHECK_ENTITY_NAME',
	'CHECK_ENTITY_X',
	'CHECK_ENTITY_Y',
	'CHECK_ENTITY_INTERACT_SCRIPT',
	'CHECK_ENTITY_TICK_SCRIPT',
	'CHECK_ENTITY_TYPE',
	'CHECK_ENTITY_PRIMARY_ID',
	'CHECK_ENTITY_SECONDARY_ID',
	'CHECK_ENTITY_PRIMARY_ID_TYPE',
	'CHECK_ENTITY_CURRENT_ANIMATION',
	'CHECK_ENTITY_CURRENT_FRAME',
	'CHECK_ENTITY_DIRECTION',
	'CHECK_ENTITY_GLITCHED',
	'CHECK_ENTITY_HACKABLE_STATE_A',
	'CHECK_ENTITY_HACKABLE_STATE_B',
	'CHECK_ENTITY_HACKABLE_STATE_C',
	'CHECK_ENTITY_HACKABLE_STATE_D',
	'CHECK_ENTITY_HACKABLE_STATE_A_U2',
	'CHECK_ENTITY_HACKABLE_STATE_C_U2',
	'CHECK_ENTITY_HACKABLE_STATE_A_U4',
	'CHECK_ENTITY_PATH',
	'CHECK_SAVE_FLAG',
	'CHECK_IF_ENTITY_IS_IN_GEOMETRY',
	'CHECK_FOR_BUTTON_PRESS',
	'CHECK_FOR_BUTTON_STATE',
	'CHECK_WARP_STATE',
	'RUN_SCRIPT',
	'BLOCKING_DELAY',
	'NON_BLOCKING_DELAY',
	'SET_ENTITY_NAME',
	'SET_ENTITY_X',
	'SET_ENTITY_Y',
	'SET_ENTITY_INTERACT_SCRIPT',
	'SET_ENTITY_TICK_SCRIPT',
	'SET_ENTITY_TYPE',
	'SET_ENTITY_PRIMARY_ID',
	'SET_ENTITY_SECONDARY_ID',
	'SET_ENTITY_PRIMARY_ID_TYPE',
	'SET_ENTITY_CURRENT_ANIMATION',
	'SET_ENTITY_CURRENT_FRAME',
	'SET_ENTITY_DIRECTION',
	'SET_ENTITY_DIRECTION_RELATIVE',
	'SET_ENTITY_DIRECTION_TARGET_ENTITY',
	'SET_ENTITY_DIRECTION_TARGET_GEOMETRY',
	'SET_ENTITY_GLITCHED',
	'SET_ENTITY_HACKABLE_STATE_A',
	'SET_ENTITY_HACKABLE_STATE_B',
	'SET_ENTITY_HACKABLE_STATE_C',
	'SET_ENTITY_HACKABLE_STATE_D',
	'SET_ENTITY_HACKABLE_STATE_A_U2',
	'SET_ENTITY_HACKABLE_STATE_C_U2',
	'SET_ENTITY_HACKABLE_STATE_A_U4',
	'SET_ENTITY_PATH',
	'SET_SAVE_FLAG',
	'SET_PLAYER_CONTROL',
	'SET_MAP_TICK_SCRIPT',
	'SET_HEX_CURSOR_LOCATION',
	'SET_WARP_STATE',
	'SET_HEX_EDITOR_STATE',
	'SET_HEX_EDITOR_DIALOG_MODE',
	'SET_HEX_EDITOR_CONTROL',
	'SET_HEX_EDITOR_CONTROL_CLIPBOARD',
	'LOAD_MAP',
	'SHOW_DIALOG',
	'PLAY_ENTITY_ANIMATION',
	'TELEPORT_ENTITY_TO_GEOMETRY',
	'WALK_ENTITY_TO_GEOMETRY',
	'WALK_ENTITY_ALONG_GEOMETRY',
	'LOOP_ENTITY_ALONG_GEOMETRY',
	'SET_CAMERA_TO_FOLLOW_ENTITY',
	'TELEPORT_CAMERA_TO_GEOMETRY',
	'PAN_CAMERA_TO_ENTITY',
	'PAN_CAMERA_TO_GEOMETRY',
	'PAN_CAMERA_ALONG_GEOMETRY',
	'LOOP_CAMERA_ALONG_GEOMETRY',
	'SET_SCREEN_SHAKE',
	'SCREEN_FADE_OUT',
	'SCREEN_FADE_IN',
	'MUTATE_VARIABLE',
	'MUTATE_VARIABLES',
	'COPY_VARIABLE',
	'CHECK_VARIABLE',
	'CHECK_VARIABLES',
	'SLOT_SAVE',
	'SLOT_LOAD',
	'SLOT_ERASE',
	'SET_CONNECT_SERIAL_DIALOG',
	'SHOW_SERIAL_DIALOG',
	'INVENTORY_GET',
	'INVENTORY_DROP',
	'CHECK_INVENTORY',
	'SET_MAP_LOOK_SCRIPT',
	'SET_ENTITY_LOOK_SCRIPT',
	'SET_TELEPORT_ENABLED',
	'CHECK_MAP',
	'SET_BLE_FLAG',
	'CHECK_BLE_FLAG',
];

var specialKeywordsEnum = {
	'%MAP%': 255,
	'%SELF%': 254,
	'%PLAYER%': 253,
	'%ENTITY_PATH%': 65535,
}

var getObjectByNameOnMap = function(name, map, action) {
	var specialIndex = specialKeywordsEnum[name];
	var object;
	if (specialIndex) {
		object = { specialIndex: specialIndex };
	} else {
		map.layers.find(function (layer) {
			const isObjectsLayer = layer.type === 'objectgroup';
			if (isObjectsLayer) {
				object = layer.objects.find(function (object) {
					return object.name === name;
				});
			}
			return object !== undefined;
		});
	}
	if (!object) {
		throw new Error(`"${action.action}" No object named "${name}" could be found on map: "${map.name}"!`);
	}
	return object;
};

var getMapLocalEntityIndexFromAction = function (
	propertyName,
	action,
	map,
	fileNameMap,
	scenarioData,
) {
	var value = action[propertyName];
	if (!value) {
		throw new Error(`${action.action} requires a string value for "${propertyName}"`);
	}
	var entity = getObjectByNameOnMap(
		value,
		map,
		action,
	);
	var mapLocalEntityIndex = (
		entity.specialIndex
		|| map.entityIndices.indexOf(entity.compositeEntity.scenarioIndex)
	)
	if(mapLocalEntityIndex === -1) {
		throw new Error(`${action.action} found entity "${value}" on map "${map.name}", but it was somehow not already a member of the map it should be used on!`);
	}
	return mapLocalEntityIndex;
};

var getEntityTypeScenarioIndex = function (
	propertyName,
	action,
	map,
	fileNameMap,
	scenarioData,
) {
	var value = action[propertyName];
	if (!value) {
		throw new Error(`${action.action} requires a string value for "${propertyName}"`);
	}
	var entityType = scenarioData.entityTypes[value];
	if(!entityType) {
		throw new Error(`${action.action} requires a valid value for "${propertyName}"; "${value}" was not found in ScenarioData!`);
	}
	return entityType.scenarioIndex;
};

var getMapIndexFromAction = function (
	propertyName,
	action,
	map,
	fileNameMap,
	scenarioData,
) {
	var value = action[propertyName];
	if (!value) {
		throw new Error(`${action.action} requires a string value for "${propertyName}"`);
	}
	var lookedUpMap = scenarioData.mapsByName[value];
	var mapIndex = lookedUpMap && lookedUpMap.scenarioIndex;
	if(mapIndex === undefined) {
		throw new Error(`${action.action} was unable to find map "${value}"!`);
	}
	return mapIndex;
};

var getGeometryIndexFromAction = function (
	propertyName,
	action,
	map,
	fileNameMap,
	scenarioData,
) {
	var value = action[propertyName];
	if (!value) {
		throw new Error(`${action.action} requires a string value for "${propertyName}"`);
	}
	var geometry = getObjectByNameOnMap(value, map, action);
	if (!geometry) {
		throw new Error(`${action.action} was not able to find geometry named "${value}" on the map named "${map.name}"`);
	}
	return geometry.specialIndex || geometry.mapIndex;
};

var buttonMap = {
	MEM0: 0,
	MEM1: 1,
	MEM2: 2,
	MEM3: 3,
	BIT128: 4,
	BIT64: 5,
	BIT32: 6,
	BIT16: 7,
	BIT8: 8,
	BIT4: 9,
	BIT2: 10,
	BIT1: 11,
	XOR: 12,
	ADD: 13,
	SUB: 14,
	PAGE: 15,
	LJOY_CENTER: 16,
	LJOY_UP: 17,
	LJOY_DOWN: 18,
	LJOY_LEFT: 19,
	LJOY_RIGHT: 20,
	RJOY_CENTER: 21,
	RJOY_UP: 22,
	RJOY_DOWN: 23,
	RJOY_LEFT: 24,
	RJOY_RIGHT: 25,
	TRIANGLE: 22,
	X: 23,
	CROSS: 23,
	CIRCLE: 24,
	O: 24,
	SQUARE: 25,
	HAX: 26, // Cap Touch
	ANY: 27, // the elusive `any key`
};

var getButtonFromAction = function (
	propertyName,
	action,
	map,
	fileNameMap,
	scenarioData,
) {
	var value = action[propertyName];
	if (value === undefined) {
		throw new Error(`${action.action} requires a value for "${propertyName}"`);
	}
	var button = buttonMap[value];
	if (button === undefined) {
		throw new Error(`${action.action} was given value "${value}", but requires a valid value for "${propertyName}"; Possible values:\n${
			Object.keys(buttonMap)
		}`);
	}
	return button;
};

var getDirectionFromAction = function (
	propertyName,
	action,
	map,
	fileNameMap,
	scenarioData,
) {
	var value = action[propertyName];
	if (value === undefined) {
		throw new Error(`${action.action} requires a value for "${propertyName}"`);
	}
	var directions = {
		0: 0,
		1: 1,
		2: 2,
		3: 3,
		"north": 0,
		"east": 1,
		"south": 2,
		"west": 3,
	};
	var direction = directions[value];
	if (direction === undefined) {
		throw new Error(`${action.action} was given value "${value}", but requires a valid value for "${propertyName}"; Possible values:\n${
			Object.keys(directions)
		}`);
	}
	return direction;
};

var getRelativeDirectionFromAction = function (
	propertyName,
	action,
	map,
	fileNameMap,
	scenarioData,
) {
	var value = action[propertyName];
	if (value === undefined) {
		throw new Error(`${action.action} requires a value for "${propertyName}"`);
	}
	if (
		!Number.isInteger(value)
		|| (Math.abs(value) > 3)
	) {
		throw new Error(`${action.action} requires a valid value for "${propertyName}"; Value must be an integer from -3 to +3`);
	}
	return value;
};

var getNumberFromAction = function (
	propertyName,
	action,
	map,
	fileNameMap,
	scenarioData
) {
	var value = action[propertyName];
	if (typeof value !== 'number') {
		throw new Error(`${action.action} requires a value for "${propertyName}"!`);
	}
	value = parseInt(value, 10);
	if (value < 0) {
		throw new Error(`${action.action} "${propertyName}" value "${value}" must be greater than or equal to zero!`);
	}
	return value;
};

var getByteFromAction = function (propertyName, action, map) {
	var value = getNumberFromAction(propertyName, action, map);
	var maxSize = 255;
	if (value > maxSize) {
		throw new Error(`${action.action} "${propertyName}" value "${value}" must be less than or equal to ${maxSize}!`);
	}
	return value;
};

var rgbRegex = /#([0-9A-Fa-f]{2})([0-9A-Fa-f]{2})([0-9A-Fa-f]{2})/;
var rgbaRegex = /#([0-9A-Fa-f]{2})([0-9A-Fa-f]{2})([0-9A-Fa-f]{2})([0-9A-Fa-f]{2})/;
var getColor = function (propertyName, action, map) {
	var value = action[propertyName];
	if (typeof value !== 'string') {
		throw new Error(`${action.action} requires a string value for "${propertyName}"!`);
	}
	var match = (
		rgbaRegex.exec(value)
		|| rgbRegex.exec(value)
	);
	if (!match) {
		throw new Error(`${action.action} "${propertyName}" value "${value}" must be greater than or equal to zero!`);
	}
	match.shift();
	match[0] = parseInt(match[0], 16);
	match[1] = parseInt(match[1], 16);
	match[2] = parseInt(match[2], 16);
	match[3] = match[3] === undefined
		? 255
		: parseInt(match[3], 16);
	return rgbaToC565(
		match[0],
		match[1],
		match[2],
		match[3],
	);
};

var getTwoBytesFromAction = function (propertyName, action, map) {
	var value = getNumberFromAction(propertyName, action, map);
	var maxSize = 65535;
	if (value > maxSize) {
		throw new Error(`${action.action} "${propertyName}" value "${value}" must be less than or equal to ${maxSize}!`);
	}
	return value;
};

var getBoolFromAction = function (
	propertyName,
	action,
	map,
	fileNameMap,
	scenarioData,
) {
	var value = action[propertyName];
	if (typeof value !== 'boolean') {
		throw new Error(`${action.action} requires a (true | false) value for "${propertyName}"!`);
	}
	return value;
};

var getStringIdFromAction = function (
	propertyName,
	action,
	map,
	fileNameMap,
	scenarioData,
) {
	var value = action[propertyName];
	if (typeof value !== 'string') {
		throw new Error(`${action.action} requires a string value for "${propertyName}"!`);
	}
	return serializeString(
		value,
		map,
		fileNameMap,
		scenarioData,
	);
};

var getSaveFlagIdFromAction = function (
	propertyName,
	action,
	map,
	fileNameMap,
	scenarioData,
) {
	var value = action[propertyName];
	if (typeof value !== 'string') {
		throw new Error(`${action.action} requires a string value for "${propertyName}"!`);
	}
	return serializeSaveFlag(
		value,
		map,
		fileNameMap,
		scenarioData,
	);
};

var getVariableIdFromAction = function (
	propertyName,
	action,
	map,
	fileNameMap,
	scenarioData,
) {
	var value = action[propertyName];
	if (typeof value !== 'string') {
		throw new Error(`${action.action} requires a string value for "${propertyName}"!`);
	}
	return serializeVariable(
		value,
		map,
		fileNameMap,
		scenarioData,
	);
};

var entityFieldMap = {
	x: 12,
	y: 14,
	interact_script_id: 16,
	tick_script_id: 18,
	primary_id: 20,
	secondary_id: 22,
	primary_id_type: 24,
	current_animation: 25,
	current_frame: 26,
	direction: 27,
	hackable_state_a: 28,
	hackable_state_b: 29,
	hackable_state_c: 30,
	hackable_state_d: 31,
};
var getFieldFromAction = function (
	propertyName,
	action,
	map,
	fileNameMap,
	scenarioData,
) {
	var value = action[propertyName];
	if (value === undefined) {
		throw new Error(`${action.action} requires a value for "${propertyName}"`);
	}
	var field = entityFieldMap[value];
	if (field === undefined) {
		throw new Error(`${action.action} was given value "${value}", but requires a valid value for "${propertyName}"; Possible values:\n${
			Object.keys(entityFieldMap)
		}`);
	}
	return field;
};

var operationMap = {
	SET: 0,
	ADD: 1,
	SUB: 2,
	DIV: 3,
	MUL: 4,
	MOD: 5,
	RNG: 6,
};
var getOperationFromAction = function (
	propertyName,
	action,
	map,
	fileNameMap,
	scenarioData,
) {
	var value = action[propertyName];
	if (value === undefined) {
		throw new Error(`${action.action} was given value "${value}", but requires a value for "${propertyName}"`);
	}
	var operation = operationMap[value];
	if (operation === undefined) {
		throw new Error(`${action.action} was given value "${value}", but requires a valid value for "${propertyName}"; Possible values:\n${
			Object.keys(operationMap)
		}`);
	}
	return operation;
};

var comparisonMap = {
	LT  : 0,
	LTEQ: 1,
	EQ  : 2,
	GTEQ: 3,
	GT  : 4,
	"<" : 0,
	"<=": 1,
	"==": 2,
	">=": 3,
	">" : 4,
};
var getComparisonFromAction = function (
	propertyName,
	action,
	map,
	fileNameMap,
	scenarioData,
) {
	var value = action[propertyName];
	if (value === undefined) {
		throw new Error(`${action.action} was given value "${value}", but requires a value for "${propertyName}"`);
	}
	var comparison = comparisonMap[value];
	if (comparison === undefined) {
		throw new Error(`${action.action} requires a valid value for "${propertyName}"; Possible values:\n${
			Object.keys(comparisonMap)
		}`);
	}
	return comparison;
};

var getDialogIdFromAction = function (
	propertyName,
	action,
	map,
	fileNameMap,
	scenarioData,
) {
	var value = action[propertyName];
	if (typeof value !== 'string') {
		throw new Error(`${action.action} requires a string value for "${propertyName}"!`);
	}
	var dialog = scenarioData.dialogs[value];
	if (!dialog) {
		throw new Error(`${action.action} was unable to find a dialog named "${value}"!`);
	}
	return serializeDialog(
		dialog,
		map,
		fileNameMap,
		scenarioData,
	);
};

var getScriptByName = function (
	scriptName,
	scenarioData,
) {
	var sourceScript = scenarioData.scripts[scriptName];
	if (!sourceScript) {
		throw new Error(`Script: "${scriptName}" could not be found in scenario.json!`);
	}
	return sourceScript;
};
var getScriptByPropertyName = function (
	propertyName,
	action,
) {
	var scriptName = action[propertyName];
	if (!scriptName) {
		throw new Error(`${action.action} requires a string value for "${propertyName}"`);
	}
	return scriptName;
};
var getMapLocalScriptIdFromAction = function (
	propertyName,
	action,
	map,
	fileNameMap,
	scenarioData,
) {
	var scriptName = getScriptByPropertyName(
		propertyName,
		action,
	);
	var encodedScript = handleScript(
		scriptName,
		map,
		fileNameMap,
		scenarioData
	);
	return encodedScript.mapLocalScriptId;
};

var initActionData = function (action) {
	var buffer = new ArrayBuffer(8);
	var dataView = new DataView(buffer);
	var actionIndex = actionNames.indexOf(action.action);
	if (actionIndex === -1) {
		throw new Error(`Invalid Action: ${action.action}`);
	}
	dataView.setUint8(
		0, // action index
		actionIndex
	);
	return {
		buffer: buffer,
		dataView: dataView,
	}
};

var getSerialDialogIdFromAction = function (
	propertyName,
	action,
	map,
	fileNameMap,
	scenarioData,
) {
	var value = action[propertyName];
	if (typeof value !== 'string') {
		throw new Error(`${action.action} requires a string value for "${propertyName}"!`);
	}
	var serialDialog = scenarioData.serialDialogs[value];
	if (!serialDialog) {
		throw new Error(`${action.action} was unable to find a serial_dialog named "${value}"!`);
	}
	return serializeSerialDialog(
		serialDialog,
		map,
		fileNameMap,
		scenarioData,
	);
};

var getItemIdFromAction = function () {
	throw new Error('getItemIdFromAction is not implemented yet!');
};

var getBleFlagIdFromAction = function () {
	throw new Error('getBleFlagIdFromAction is not implemented yet!');
};


var actionPropertyNameToHandlerMap = {
	duration: getNumberFromAction,
	expected_u4: getNumberFromAction,
	map: getMapIndexFromAction,
	entity: getMapLocalEntityIndexFromAction,
	target_entity: getMapLocalEntityIndexFromAction,
	entity_type: getEntityTypeScenarioIndex,
	geometry: getGeometryIndexFromAction,
	target_geometry: getGeometryIndexFromAction,
	script: getMapLocalScriptIdFromAction,
	success_script: getMapLocalScriptIdFromAction,
	expected_script: getMapLocalScriptIdFromAction,
	string: getStringIdFromAction,
	save_flag: getSaveFlagIdFromAction,
	dialog: getDialogIdFromAction,
	address: getTwoBytesFromAction,
	color: getColor,
	expected_u2: getTwoBytesFromAction,
	u2_value: getTwoBytesFromAction,
	amplitude: getByteFromAction,
	bitmask: getByteFromAction,
	button_id: getButtonFromAction,
	byte_offset: getByteFromAction,
	byte_value: getByteFromAction,
	expected_byte: getByteFromAction,
	animation: getByteFromAction,
	play_count: getByteFromAction,
	frequency: getTwoBytesFromAction,
	font_id: getByteFromAction,
	slot: getByteFromAction,
	direction: getDirectionFromAction,
	relative_direction: getRelativeDirectionFromAction,
	bool_value: getBoolFromAction,
	expected_bool: getBoolFromAction,
	value: getTwoBytesFromAction,
	variable: getVariableIdFromAction,
	source: getVariableIdFromAction,
	field: getFieldFromAction,
	inbound: getBoolFromAction,
	operation: getOperationFromAction,
	comparison: getComparisonFromAction,
	serial_dialog: getSerialDialogIdFromAction,
	item_name: getItemIdFromAction,
	ble_flag: getBleFlagIdFromAction,
};

var sizeHandlerMap = [
	'BAD_SIZE_ERROR',
	'setUint8',
	'setUint16',
	'BAD_SIZE_ERROR',
	'setUint32',
	'BAD_SIZE_ERROR',
	'BAD_SIZE_ERROR',
	'BAD_SIZE_ERROR',
	'BAD_SIZE_ERROR',
];

var handleActionWithFields = function(
	action,
	fields,
	map,
	fileNameMap,
	scenarioData,
) {
	var data = initActionData(action);
	var offset = 1; // always start at 1 because that's the actionId
	fields.forEach(function (field) {
		var handler = actionPropertyNameToHandlerMap[field.propertyName];
		if (!handler) {
			throw new Error(`No action field handler for property "${field.propertyName}"!`)
		}
		var value = handler(
			field.propertyName,
			action,
			map,
			fileNameMap,
			scenarioData,
		);
		var dataViewMethodName = sizeHandlerMap[field.size];
		data.dataView[dataViewMethodName](
			offset,
			value,
			field.endian === undefined
				? IS_LITTLE_ENDIAN
				: field.endian,
		);
		offset += field.size;
	})
	return data;
};

var serializeAction = function (
	action,
	map,
	fileNameMap,
	scenarioData,
) {
	var actionIndex = actionNames.indexOf(action.action);
	if (actionIndex === -1) {
		throw new Error(`Action: "${action.action}" is not valid! Check the "actionHandlerMap" for valid options!`);
	}
	var fields = actionFieldsMap[action.action];
	if (!fields) {
		throw new Error(`Action: "${action.action}" has not been implemented yet! Please add it to the "actionHandlerMap"!`);
	}
	return handleActionWithFields(
		action,
		fields,
		map,
		fileNameMap,
		scenarioData,
	).buffer;
};

var detectCopyScript = function (script) {
	return script.filter(function (action) {
		return action.action === 'COPY_SCRIPT';
	}).length > 0;
};

var preProcessScript = function(
	script,
	scriptName,
	map,
	fileNameMap,
	scenarioData,
) {
	var result = script;
	var read = script;
	while (detectCopyScript(read)) {
		result = [];
		read.forEach(function (action) {
			if(action.action === 'COPY_SCRIPT') {
				var scriptName = getScriptByPropertyName(
					'script',
					action,
				);
				var sourceScript = getScriptByName(
					scriptName,
					scenarioData
				);
				var copiedScript = jsonClone(sourceScript);
				result = result.concat(copiedScript);
			} else {
				result.push(action);
			}
		});
		read = result;
	}
	return result;
};

var serializeScript = function (
	script,
	scriptName,
	map,
	fileNameMap,
	scenarioData,
) {
	var headerLength = (
		32 // char[32] name
		+ 4 // uint32_t action_count
	);
	var result = new ArrayBuffer(
		getPaddedHeaderLength(headerLength)
	);
	var dataView = new DataView(result);
	var offset = 0;
	setCharsIntoDataView(
		dataView,
		scriptName,
		0,
		offset += 32
	);
	var compositeScript = preProcessScript(
		script,
		scriptName,
		map,
		fileNameMap,
		scenarioData,
	);
	dataView.setUint32(
		offset,
		compositeScript.length,
		IS_LITTLE_ENDIAN
	);
	offset += 4;

	// in case actions call scripts that call this script again,
	// put this script into the scriptKeyNames first,
	// so others can refer to this without infinite looping because
	// it's already in there.
	script.scenarioIndex = scenarioData.parsed.scripts.length;
	scenarioData.parsed.scripts.push(script);
	var mapLocalScriptId = map.scriptIndices.length;
	map.scriptIndices.push(script.scenarioIndex);
	map.scriptNameKeys[scriptName] = {
		compositeScript: compositeScript,
		mapLocalScriptId: mapLocalScriptId,
		globalScriptId: script.scenarioIndex
	};

	compositeScript.forEach(function(action) {
		result = combineArrayBuffers(
			result,
			serializeAction(
				action,
				map,
				fileNameMap,
				scenarioData,
			),
		);
	});
	return result;
};

var serializeNullScript = function(
	fileNameMap,
	scenarioData,
) {
	var nullScript = [];
	nullScript.serialized = serializeScript(
		nullScript,
		'null_script',
		{
			name: 'null_map_only_used_for_null_script',
			scriptIndices: [],
			scriptNameKeys: {},
		},
		fileNameMap,
		scenarioData,
	);
	scenarioData.scripts['null_script'] = nullScript;
}

var handleScript = function(
	scriptName,
	map,
	fileNameMap,
	scenarioData,
) {
	var result = map.scriptNameKeys[scriptName];
	if (!result) {
		if(scriptName === 'null_script') {
			result = {
				mapLocalScriptId: 0,
				globalScriptId: 0
			};
			map.scriptIndices.push(0);
			map.scriptNameKeys[scriptName] = result;
		} else {
			var sourceScript = getScriptByName(
				scriptName,
				scenarioData,
			);
			var script = jsonClone(sourceScript);
			script.serialized = serializeScript(
				script,
				scriptName,
				map,
				fileNameMap,
				scenarioData,
			);
			result = map.scriptNameKeys[scriptName];
		}
	}
	return result;
};

var possibleEntityScripts = ['on_interact', 'on_tick'];

var handleMapEntityScripts = function (
	map,
	fileNameMap,
	scenarioData,
) {
	map.entityIndices.forEach(function (globalEntityIndex) {
		var entity = scenarioData.parsed.entities[globalEntityIndex];
		possibleEntityScripts.forEach(function (propertyName) {
			var scriptName = entity[propertyName];
			map.currentEntityMapIndex = entity.mapIndex;
			if (scriptName) {
				var mapLocalScriptId = handleScript(
					scriptName,
					map,
					fileNameMap,
					scenarioData,
				).mapLocalScriptId;
				entity.dataView.setUint16(
					entity.dataView[propertyName + '_offset'], // uint16_t on_${possibleScriptName}_script_id
					mapLocalScriptId,
					IS_LITTLE_ENDIAN
				);
			}
		});
		map.currentEntityMapIndex = undefined;
	});
};

var possibleMapScripts = [
	'on_load',
	'on_tick',
	'on_look',
];

var collectMapScripts = function (
	map,
) {
	var result = {};
	// this is the shape if it came from a tiled map file
	(map.properties || []).forEach(function(property) {
		if (
			property.value // because if it's empty, don't bother
			&& possibleMapScripts.includes(property.name)
		) {
			result[property.name] = property.value;
		}
	});
	// this is if it's a property defined in maps.json
	possibleMapScripts.forEach(function (scriptSlot) {
		var scriptName = map[scriptSlot];
		var existingScriptName = result[scriptSlot];
		if (scriptName) {
			if (existingScriptName) {
				throw new Error(`Duplicate "${scriptSlot}" definition on map "${map.name}": Your map has this script defined in the Tiled map, as well as maps.json. Remove one of them to continue.`);
			} else {
				result[scriptSlot] = scriptName;
			}
		}
	});
	return result;
};

var handleMapScripts = function (
	map,
	fileNameMap,
	scenarioData,
) {
	//  {
	//	"name":"on_load",
	//	"type":"string",
	//	"value":"my_first_script"
	//  },
	handleScript( // add the global null_script id to the local map scripts
		'null_script',
		map,
		fileNameMap,
		scenarioData,
	);
	var mapScripts = collectMapScripts(map);
	// console.log(`Processing scripts for map: "${map.name}"`);
	Object.keys(mapScripts).forEach(function (scriptSlot) {
		var scriptName = mapScripts[scriptSlot];
		// console.log(`	- ${scriptSlot}:${scriptName}`);
		map[scriptSlot] = handleScript(
			scriptName,
			map,
			fileNameMap,
			scenarioData,
		).mapLocalScriptId;
	});
	map.directionScriptIds = {};
	Object.keys(map.directions || {}).forEach(function (directionName) {
		var scriptName = map.directions[directionName];
		map.directionScriptIds[directionName] = handleScript(
			scriptName,
			map,
			fileNameMap,
			scenarioData,
		).mapLocalScriptId;
	});
	handleMapEntityScripts(
		map,
		fileNameMap,
		scenarioData,
	);
};

var makeVariableLookaheadFunction = function(scenarioData) {
	return function (script) {
		script.forEach(function (action) {
			if(action.variable) {
				serializeVariable(action.variable, {}, {}, scenarioData);
			}
			if(action.source) {
				serializeVariable(action.source, {}, {}, scenarioData);
			}
		});
	}
};
var dialogAlignmentEnum = {
	BOTTOM_LEFT: 0,
	BOTTOM_RIGHT: 1,
	TOP_LEFT: 2,
	TOP_RIGHT: 3,
};
var dialogResponseTypeEnum = {
	NO_RESPONSE: 0,
	SELECT_FROM_SHORT_LIST: 1,
	SELECT_FROM_LONG_LIST: 2,
	ENTER_NUMBER: 3,
	ENTER_ALPHANUMERIC: 4
};

var serializeDialog = function (
	dialog,
	map,
	fileNameMap,
	scenarioData,
) {
	if (
		!Array.isArray(dialog)
		|| !dialog.length
	) {
		throw new Error(`Dialog named "${dialog.name}" is malformed, it has no dialogScreens!`);
	}
	dialog.forEach(function (dialogScreen, index) {
		if(
			!Array.isArray(dialogScreen.messages)
			|| !dialogScreen.messages.length
		) {
			throw new Error(`Dialog named "${dialog.name}" is malformed, the ${index}th dialogScreen contains no messages!`);
		}
	});
	var uniqueMapAndDialogKey = `map:${
		map.name.replace('map-', '')
	},dialog:${
		dialog.name.replace('dialog-', '')
	}`;
	var scenarioIndex = scenarioData.uniqueDialogMap[uniqueMapAndDialogKey];
	if(scenarioIndex === undefined) {
		var headerLength = getPaddedHeaderLength(
			32 // char[32] name
			+ 4 // uint32_t screen_count
		);
		var result = new ArrayBuffer(
			headerLength
		);
		var dataView = new DataView(result);
		var offset = 0;
		setCharsIntoDataView(
			dataView,
			uniqueMapAndDialogKey,
			0,
			offset += 32
		);
		dataView.setUint32(
			offset, // uint32_t screen_count
			dialog.length,
			IS_LITTLE_ENDIAN
		);
		offset += 4;
		dialog.forEach(function (dialogScreen) {
			result = combineArrayBuffers(
				result,
				serializeDialogScreen(
					dialogScreen,
					map,
					fileNameMap,
					scenarioData,
				),
			);
		});
		var encodedDialog = {
			name: uniqueMapAndDialogKey,
			serialized: result,
			scenarioIndex: scenarioData.parsed.dialogs.length,
		}
		scenarioData.parsed.dialogs.push(encodedDialog);
		scenarioIndex = encodedDialog.scenarioIndex;
		scenarioData.uniqueDialogMap[uniqueMapAndDialogKey] = scenarioIndex;
	}
	return scenarioIndex;
};

var getPortraitIndexFromDialogScreen = function(
	dialogScreen,
	scenarioData
) {
	var portrait = scenarioData.portraits[dialogScreen.portrait];
	if(dialogScreen.portrait && !portrait) {
		throw new Error(`DialogScreen referenced a invalid portrait name: ${dialogScreen.portrait}!\nDialog was: ${JSON.stringify(dialogScreen, null, '\t')}`);
	}
	return dialogScreen.portrait
		? portrait.scenarioIndex
		: DIALOG_SCREEN_NO_PORTRAIT
}

var serializeDialogScreen = function (
	dialogScreen,
	map,
	fileNameMap,
	scenarioData,
) {
	var responses = dialogScreen.options || [];
	var entity = dialogScreen.entity
		? getObjectByNameOnMap(
			dialogScreen.entity,
			map,
			{ action: "serializeDialogScreen" },
		)
		: null;
	var entityIndex = dialogScreen.entity
		? (entity.specialIndex || entity.mapIndex)
		: DIALOG_SCREEN_NO_ENTITY;
	var portraitIndex = getPortraitIndexFromDialogScreen(
		dialogScreen,
		scenarioData
	);
	var name = (
		dialogScreen.name
		|| (
			entityIndex
				? dialogScreen.entity
				: ""
		)
	);
	var headerLength = getPaddedHeaderLength(
		+ 2 // uint16_t name_string_index
		+ 2 // uint16_t border_tileset_index
		+ 1 // uint8_t alignment
		+ 1 // uint8_t font_index
		+ 1 // uint8_t message_count
		+ 1 // uint8_t response_type
		+ 1 // uint8_t response_count
		+ 1 // uint8_t entity_id
		+ 1 // uint8_t portrait_id
		+ 1 // uint8_t emote
		+ (2 * dialogScreen.messages.length) // uint16_t messages[message_count]
		+ (
			(
				+ 2 //stringIndex
				+ 2 //mapLocalScriptIndex
			)
			* responses.length
		) // uint16_t responses[response_count]
	);
	var result = new ArrayBuffer(
		headerLength
	);
	var dataView = new DataView(result);
	var offset = 0;
	var nameStringId = serializeString(
		name,
		map,
		fileNameMap,
		scenarioData,
	);
	dataView.setUint16(
		offset, // uint16_t name_index
		nameStringId,
		IS_LITTLE_ENDIAN
	);
	offset += 2;
	var dialogTilesetFilePath = scenarioData.dialogSkins[dialogScreen.border_tileset || 'default'];
	var borderTileset = getPreloadedTilesetByName(
		dialogTilesetFilePath,
		fileNameMap,
		scenarioData,
	);
	dataView.setUint16(
		offset, // uint16_t border_tileset_index
		borderTileset.scenarioIndex,
		IS_LITTLE_ENDIAN
	);
	offset += 2;
	dataView.setUint8(
		offset, // uint8_t alignment
		dialogAlignmentEnum[dialogScreen.alignment] || 0, // TODO: Make MORE use of ksy enum `dialog_screen_alignment_type`
	);
	offset += 1;
	dataView.setUint8(
		offset, // uint8_t font_index
		0, // TODO: add font_index support
	);
	offset += 1;
	dataView.setUint8(
		offset, // uint8_t message_count
		dialogScreen.messages.length,
	);
	offset += 1;
	dataView.setUint8(
		offset, // uint8_t response_type
		dialogResponseTypeEnum[dialogScreen.response_type] || 0,
	);
	offset += 1;
	dataView.setUint8(
		offset, // uint8_t response_count
		responses.length,
	);
	offset += 1;
	dataView.setUint8(
		offset, // uint8_t entity_id
		entityIndex,
	);
	offset += 1;
	dataView.setUint8(
		offset, // uint8_t portrait_id
		portraitIndex,
	);
	offset += 1;
	dataView.setUint8(
		offset, // uint8_t emote
		dialogScreen.emote || 0,
	);
	offset += 1;
	dialogScreen.messages.forEach(function (message) {
		var stringId = serializeString(
			message,
			map,
			fileNameMap,
			scenarioData,
		);
		dataView.setUint16(
			offset, // uint16_t string_id
			stringId,
			IS_LITTLE_ENDIAN
		);
		offset += 2;
	});
	responses.forEach(function (response) {
		var stringId = serializeString(
			response.label,
			map,
			fileNameMap,
			scenarioData,
		);
		var encodedScript = handleScript(
			response.script,
			map,
			fileNameMap,
			scenarioData
		);
		dataView.setUint16(
			offset, // uint16_t string_id
			stringId,
			IS_LITTLE_ENDIAN
		);
		offset += 2;
		dataView.setUint16(
			offset, // uint16_t string_id
			encodedScript.mapLocalScriptId,
			IS_LITTLE_ENDIAN
		);
		offset += 2;
	});
	return result;
};

var preloadAllDialogSkins = function (filenameMap, scenarioData) {
	return Promise.all(Object.keys(scenarioData.dialogSkins).map((function(key) {
		return loadTilesetByName(
			scenarioData.dialogSkins[key],
			filenameMap,
			scenarioData,
		)
			.then(function (tilesetData) {
				scenarioData.dialogSkinsTilesetMap[key] = tilesetData;
			});
	})));
};
var serialDialogResponseTypeEnum = {
	NONE: 0,
	ENTER_NUMBER: 1,
	ENTER_STRING: 2
};

var emptyMap = { // global scope, mock being real map
	name: 'GLOBAL',
	scriptIndices: [],
	scriptNameKeys: {},
	layers: []
}

var serializeSerialDialog = function (
	serialDialog,
	map,
	fileNameMap,
	scenarioData,
) {
	if(
		!Array.isArray(serialDialog.messages)
		|| !serialDialog.messages.length
	) {
		throw new Error(`SerialDialog named "${serialDialog.name}" is malformed, it contains no messages!`);
	}
	var uniqueSerialDialogKey = serialDialog.name.replace('serial_dialog-', '');
	var scenarioIndex = serialDialog.scenarioIndex;
	if(scenarioIndex === undefined) {
		var responses = serialDialog.options || [];
		var responseType = serialDialogResponseTypeEnum.NONE;
		if (responses.length && serialDialog.text_options) {
			throw new Error(`SerialDialog named "${serialDialog.name}" is malformed, it has both 'options' and
'text_options'! Pick one, not both!`);
		}
		if (responses.length) {
			responseType = serialDialogResponseTypeEnum.ENTER_NUMBER;
		}
		var textOptionKeys = Object.keys(serialDialog.text_options || {});
		if (textOptionKeys.length) {
			responses = textOptionKeys.map(function (key) {
				return {
					label: key.toLocaleLowerCase(),
					script: serialDialog.text_options[key],
				};
			});
			responseType = serialDialogResponseTypeEnum.ENTER_STRING;
		}
		if (responses.length > 255) {
			throw new Error(`SerialDialog named "${serialDialog.name}" is malformed, it has too many options! It should have less than 265 options!!`);
		}
		var headerLength = getPaddedHeaderLength(
			32 // char[32] name
			+ 2 // uint16_t string_id
			+ 1 // uint8_t response_type
			+ 1 // uint8_t response_count
			+ (
				(
					+ 2 //string_id
					+ 2 //map_local_script_id
				)
				* responses.length
			) // responses[response_count]
		);
		var result = new ArrayBuffer(
			headerLength
		);
		var dataView = new DataView(result);
		var offset = 0;
		setCharsIntoDataView(
			dataView,
			uniqueSerialDialogKey,
			0,
			offset += 32
		);
		var stringId = serializeString(
			serialDialog.messages.join('\n'),
			map,
			fileNameMap,
			scenarioData,
		);
		dataView.setUint16(
			offset, // uint16_t string_id
			stringId,
			IS_LITTLE_ENDIAN
		);
		offset += 2;
		dataView.setUint8(
			offset, // uint8_t response_type
			responseType || 0,
		);
		offset += 1;
		dataView.setUint8(
			offset, // uint8_t response_count
			responses.length,
		);
		offset += 1;
		responses.forEach(function (response) {
			var stringId = serializeString(
				response.label,
				map,
				fileNameMap,
				scenarioData,
			);
			var encodedScript = handleScript(
				response.script,
				map,
				fileNameMap,
				scenarioData
			);
			dataView.setUint16(
				offset, // uint16_t string_id
				stringId,
				IS_LITTLE_ENDIAN
			);
			offset += 2;
			dataView.setUint16(
				offset, // uint16_t map_local_script_id
				encodedScript.mapLocalScriptId,
				IS_LITTLE_ENDIAN
			);
			offset += 2;
		});
		var encodedSerialDialog = {
			name: uniqueSerialDialogKey,
			serialized: result,
			scenarioIndex: scenarioData.parsed.serialDialogs.length,
		}
		scenarioData.parsed.serialDialogs.push(encodedSerialDialog);
		scenarioIndex = encodedSerialDialog.scenarioIndex;
	}
	return scenarioIndex;
};
var rgbaToC565 = function (r, g, b, a) {
	return a < 100
		? 32
		: (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
};

var serializeColorPalette = function (colorPalette) {
	var colors = colorPalette.colorArray;
	var name = colorPalette.name;
	var headerLength = getPaddedHeaderLength(
		32 // name
		+ 1 // uint8_t color_count
		+ 1 // uint8_t padding
		+ 2 * colors.length // uint16_t colors
	);
	var arrayBuffer = new ArrayBuffer(headerLength);
	var dataView = new DataView(arrayBuffer);
	var offset = 0;
	setCharsIntoDataView(
		dataView,
		name,
		0,
		offset += 32
	);
	dataView.setUint8(
		offset, // uint8_t color_count
		colors.length
	);
	offset += 1;
	offset += 1; // uint8_t padding
	colors.forEach(function (color) {
		// DO NOT USE `IS_LITTLE_ENDIAN` HERE!
		// The screen _hardware_ is Big Endian,
		// and converting the whole framebuffer each tick would cost us 20ms
		dataView.setUint16(
			offset, // uint16_t color
			parseInt(color, 10),
			IS_SCREEN_LITTLE_ENDIAN
		);
		offset += 2;
	});
	return arrayBuffer;
};

var imageTypeHandlerMap = {
	gif: function(fileUint8Buffer) {
		var reader = new window.omggif.GifReader(fileUint8Buffer);
		var info = reader.frameInfo(0);
		var buffer = new Uint8Array(
			info.width * info.height * 4
		);
		reader.decodeAndBlitFrameRGBA(0, buffer);
		return {
			info: {
				width: info.width,
				height: info.height,
				channels: 4
			},
			data: buffer
		};
	},
	png: function(fileUint8Buffer) {
		var decoded = window.fastPng.decode(fileUint8Buffer);
		var info = {
			width: decoded.width,
			height: decoded.height,
			channels: decoded.channels
		};
		var buffer = decoded.data;
		if (decoded.palette) {
			info.channels = decoded.palette[0].length;
			var hasAlpha = info.channels === 4;
			var pixels = info.width * info.height;
			buffer = new Uint8Array(pixels * info.channels);
			for (var i = 0; i < pixels; i++) {
				var offset = i * info.channels;
				var colorIndex = decoded.data[i];
				var color = decoded.palette[colorIndex] || [0, 0, 0, 0];
				buffer[offset + 0] = color[0];
				buffer[offset + 1] = color[1];
				buffer[offset + 2] = color[2];
				if (hasAlpha) {
					buffer[offset + 3] = color[3];
				}
			}
		}
		return {
			info: info,
			data: buffer
		}
	},
};
var supportedImageTypes = Object.keys(imageTypeHandlerMap);

var handleImage = function (tileset, scenarioData, fileNameMap) {
	var imageFileName = tileset.image;
	var file = fileNameMap[imageFileName.split('/').pop()];
	var result = Promise.resolve(file);
	if (file.scenarioIndex === undefined) {
		if (window.Navigator) {
			// node doesn't have a createObjectURL, and we need this for
			// UI components to display these images
			console.log('DOES NODE HAVE URL.createObjectURL???')
			file.blobUrl = URL.createObjectURL(file);
		}
		var mimeTypeSuffix = file.type.split('/').pop();
		var imageHandler = imageTypeHandlerMap[mimeTypeSuffix];
		if (!imageHandler) {
			throw new Error(
				'Unsupported image type: "'
				+ file.type
				+ '" detected in file named: "'
				+ file.name
				+ '". Supported types are: '
				+ supportedImageTypes.join()
			);
		}
		file.scenarioIndex = scenarioData.parsed.images.length;
		scenarioData.parsed.images.push(file);
		var colorPalette = {
			name: file.name,
			colorArray: [],
			serialized: null,
			scenarioIndex: scenarioData.parsed.imageColorPalettes.length,
		};
		scenarioData.parsed.imageColorPalettes.push(colorPalette);
		result = file.arrayBuffer()
			.then(function (arrayBuffer) {
				return imageHandler(new Uint8Array(arrayBuffer));
			})
			.then(function (result) {
				var getPaletteIndexForColor = function (color) {
					var colorIndex = colorPalette.colorArray.indexOf(color);
					if (colorIndex === -1) {
						colorIndex = colorPalette.colorArray.length;
						colorPalette.colorArray.push(color);
					}
					return colorIndex;
				};
				var sourceWidth = result.info.width;
				var sourceHeight = result.info.height;
				var pixelsPerTile = tileset.tilewidth * tileset.tileheight;
				var channels = result.info.channels;
				var hasAlpha = channels === 4;
				var dataLength = sourceWidth * sourceHeight;
				var data = new ArrayBuffer(dataLength);
				var dataView = new DataView(data);
				var pixelIndex = 0;
				// var wtfLog = [];
				while (pixelIndex < dataLength) {
					var readOffset = pixelIndex * channels;
					var sourceX = pixelIndex % sourceWidth;
					var sourceY = Math.floor(pixelIndex / sourceWidth);
					var tileX = sourceX % tileset.tilewidth;
					var tileY = sourceY % tileset.tileheight;
					var column = Math.floor(sourceX / tileset.tilewidth);
					var row = Math.floor(sourceY / tileset.tileheight);
					var writeOffset = (
						tileX
						+ (tileY * tileset.tilewidth)
						+ (((row * tileset.columns) + column) * pixelsPerTile)
					);
					var rgba = {
						r: result.data[readOffset],
						g: result.data[readOffset + 1],
						b: result.data[readOffset + 2],
						a: hasAlpha
							? result.data[readOffset + 3]
							: 255
					};
					var color = rgbaToC565(
						rgba.r,
						rgba.g,
						rgba.b,
						rgba.a,
					);
					var paletteIndex = getPaletteIndexForColor(color);
					if (paletteIndex > 255) {
						throw new Error(`"${imageFileName}" has too many colors! Max supported colors are 256.`);
					}
					// if (paletteIndex > 255) {
					// 	wtfLog.push({
					// 		pixelIndex,
					// 		sourceX,
					// 		sourceY,
					// 		rgba: JSON.stringify(rgba),
					// 		color,
					// 		paletteIndex,
					// 	});
					// }
					dataView.setUint8(
						writeOffset,
						paletteIndex
					);
					pixelIndex += 1;
				}
				// console.table(wtfLog);
				console.log(`Colors in image "${imageFileName}": ${colorPalette.colorArray.length}`);
				file.serialized = data;
				colorPalette.serialized = serializeColorPalette(colorPalette);
				return file;
			});
	}
	return result;
};
var dataTypes = [
	'maps',
	'tilesets',
	'animations',
	'entityTypes',
	'entities',
	'geometry',
	'scripts',
	'portraits',
	'dialogs',
	'serialDialogs',
	'imageColorPalettes',
	'strings',
	'save_flags',
	'variables',
	'images',
];

var handleScenarioData = function (fileNameMap) {
	return function (scenarioData) {
		console.log(
			'scenario.json',
			scenarioData
		);
		scenarioData.tilesetMap = {};
		scenarioData.mapsByName = {};
		scenarioData.parsed = {};
		scenarioData.uniqueStringLikeMaps = {
			strings: {},
			save_flags: {},
			variables: {},
		};
		scenarioData.uniqueDialogMap = {};
		scenarioData.entityTypesPlusProperties = {};
		dataTypes.forEach(function (typeName) {
			scenarioData.parsed[typeName] = [];
		});
		scenarioData.dialogSkinsTilesetMap = {}
		var preloadSkinsPromise = preloadAllDialogSkins(fileNameMap, scenarioData);
		var portraitsFile = fileNameMap['portraits.json'];
		var portraitsPromise = preloadSkinsPromise.then(function () {
			return getFileJson(portraitsFile)
				.then(handlePortraitsData(fileNameMap, scenarioData))
		});
		var entityTypesFile = fileNameMap['entity_types.json'];
		var entityTypesPromise = portraitsPromise.then(function () {
			return getFileJson(entityTypesFile)
				.then(handleEntityTypesData(fileNameMap, scenarioData))
		});

		var mergeNamedJsonIntoScenario = function (
			pathPropertyName,
			destinationPropertyName,
			mergeSuccessCallback
		) {
			return function (
				fileNameMap,
				scenarioData,
			) {
				var collectedTypeMap = {};
				var itemSourceFileMap = {};
				var fileItemMap = {};
				scenarioData[destinationPropertyName] = collectedTypeMap;
				scenarioData[destinationPropertyName + 'SourceFileMap'] = itemSourceFileMap;
				scenarioData[destinationPropertyName + 'FileItemMap'] = fileItemMap;
				var result = Promise.all(
					scenarioData[pathPropertyName].map(function(filePath) {
						var fileName = filePath.split('/').pop();
						var fileObject = fileNameMap[fileName];
						return getFileJson(fileObject)
							.then(function(fileData) {
								Object.keys(fileData)
									.forEach(function(itemName, index) {
										if (collectedTypeMap[itemName]) {
											throw new Error(`Duplicate ${destinationPropertyName} name "${itemName}" found in ${fileName}!`);
										}
										fileData[itemName].name = itemName;
										collectedTypeMap[itemName] = fileData[itemName];
										itemSourceFileMap[itemName] = {
											fileName: fileName,
											index: index
										};
										if (!fileItemMap[fileName]) {
											fileItemMap[fileName] = [];
										}
										fileItemMap[fileName].push(
											itemName
										);
									});
							});
					})
				)
					.then(function () {
						return collectedTypeMap;
					});
				if (mergeSuccessCallback) {
					result = result.then(mergeSuccessCallback);
				}
				return result;
			}
		};

		var mergeScriptDataIntoScenario = mergeNamedJsonIntoScenario(
			'scriptPaths',
			'scripts',
			function (allScripts) {
				var lookaheadAndIdentifyAllScriptVariables = makeVariableLookaheadFunction(scenarioData);
				Object.values(allScripts)
					.forEach(lookaheadAndIdentifyAllScriptVariables);
			}
		);
		var mergeDialogDataIntoScenario = mergeNamedJsonIntoScenario(
			'dialogPaths',
			'dialogs',
		);
		var mergeSerialDialogDataIntoScenario = mergeNamedJsonIntoScenario(
			'serialDialogPaths',
			'serialDialogs',
		);
		return Promise.all([
			entityTypesPromise,
			mergeScriptDataIntoScenario(fileNameMap, scenarioData),
			mergeDialogDataIntoScenario(fileNameMap, scenarioData),
			mergeSerialDialogDataIntoScenario(fileNameMap, scenarioData),
			mergeMapDataIntoScenario(fileNameMap, scenarioData),
		])
			.then(function () {
				serializeNullScript(
					fileNameMap,
					scenarioData,
				);
				return handleScenarioMaps(scenarioData, fileNameMap)
					.then(function () {
						return scenarioData;
					});
			});
	}
};

var addParsedTypeToHeadersAndChunks = function (
	parsedItems,
	indicesDataView,
	chunks
) {
	indicesDataView.setUint32(
		indicesDataView.headerOffset,
		parsedItems.length,
		IS_LITTLE_ENDIAN
	);
	indicesDataView.headerOffset += 4;
	parsedItems.forEach(function (item, index, list) {
		var headerOffsetOffset = indicesDataView.headerOffset + (index * 4);
		var headerLengthOffset = (
			headerOffsetOffset
			+ (list.length * 4)
		);
		var totalSize = 0;
		indicesDataView.setUint32(
			headerOffsetOffset,
			indicesDataView.fileOffset,
			IS_LITTLE_ENDIAN
		);
		chunks.push(item.serialized);
		totalSize += item.serialized.byteLength;
		indicesDataView.setUint32(
			headerLengthOffset,
			totalSize,
			IS_LITTLE_ENDIAN
		);
		indicesDataView.fileOffset += totalSize;
	});
	indicesDataView.headerOffset += parsedItems.length * 8;
};

var makeCRCTable = function(){
	var c;
	var crcTable = [];
	for(var n =0; n < 256; n++){
		c = n;
		for(var k =0; k < 8; k++){
			c = ((c&1) ? (0xEDB88320 ^ (c >>> 1)) : (c >>> 1));
		}
		crcTable[n] = c;
	}
	return crcTable;
}

var crc32 = function(data) {
	var crcTable = window.crcTable || (window.crcTable = makeCRCTable());
	var crc = 0 ^ (-1);
	for (var i = 0; i < data.length; i++ ) {
		crc = (crc >>> 8) ^ crcTable[(crc ^ data[i]) & 0xFF];
	}
	return (crc ^ (-1)) >>> 0;
};

var generateIndexAndComposite = function (scenarioData) {
	// console.log(
	// 	'generateIndexAndComposite:scenarioData',
	// 	scenarioData
	// );
	var signature = new ArrayBuffer(20);
	var signatureDataView = new DataView(signature);
	setCharsIntoDataView(
		signatureDataView,
		'MAGEGAME',
		0
	);
	var headerSize = 0;
	dataTypes.forEach(function (typeName) {
		headerSize += (
			4 // uint32_t count
			+ (4 * scenarioData.parsed[typeName].length) // uint32_t offsets
			+ (4 * scenarioData.parsed[typeName].length) // uint32_t lengths
		)
	});
	var indices = new ArrayBuffer(headerSize);
	var indicesDataView = new DataView(indices);
	var chunks = [
		signature,
		indices
	];
	indicesDataView.fileOffset = signature.byteLength + indices.byteLength;
	indicesDataView.headerOffset = 0;

	dataTypes.forEach(function (type) {
		addParsedTypeToHeadersAndChunks(
			scenarioData.parsed[type],
			indicesDataView,
			chunks
		);
	})

	var compositeSize = chunks.reduce(
		function (accumulator, item) {
			return accumulator + item.byteLength;
		},
		0
	);
	var compositeArray = new Uint8Array(compositeSize);
	var currentOffset = 0;
	chunks.forEach(function (item) {
		compositeArray.set(
			new Uint8Array(item),
			currentOffset
		);
		currentOffset += item.byteLength;
	});

	var compositeArrayDataView = new DataView(compositeArray.buffer);
	var compositeArrayDataViewOffsetBySignature = new Uint8Array(
		compositeArray.buffer,
		signature.byteLength
	);
	var checksum = crc32(compositeArrayDataViewOffsetBySignature);
	compositeArrayDataView.setUint32(
		8,
		ENGINE_VERSION,
		IS_LITTLE_ENDIAN
	);
	compositeArrayDataView.setUint32(
		12,
		checksum,
		IS_LITTLE_ENDIAN
	);
	compositeArrayDataView.setUint32(
		16,
		compositeSize,
		IS_LITTLE_ENDIAN
	);

	console.log(
		'compositeArray',
		compositeArray
	);
	var hashHex = [
		compositeArray[12],
		compositeArray[13],
		compositeArray[14],
		compositeArray[15],
	].map(function (value) {
		return value.toString(16).padStart(2, 0)
	}).join('');
	var lengthHex = [
		compositeArray[16],
		compositeArray[17],
		compositeArray[18],
		compositeArray[19],
	].map(function (value) {
		return value.toString(16).padStart(2, 0)
	}).join('');
	console.log('data crc32:', checksum);
	console.log('data crc32 hex:', hashHex);
	console.log('data length:', compositeSize);
	console.log('data length hex:', lengthHex);
	return compositeArray;
};
