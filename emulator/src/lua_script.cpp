/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */
 
#include <stdio.h>
#include "lua_script.h"
#include "lodepng.h"
#include "color.h"
#include "svmmemory.h"
#include "flash_volume.h"
#include "flash_volumeheader.h"

System *LuaSystem::sys = NULL;
const char LuaSystem::className[] = "System";
const char LuaFrontend::className[] = "Frontend";
const char LuaCube::className[] = "Cube";
const char LuaRuntime::className[] = "Runtime";
const char LuaFilesystem::className[] = "Filesystem";


Lunar<LuaSystem>::RegType LuaSystem::methods[] = {
    LUNAR_DECLARE_METHOD(LuaSystem, init),
    LUNAR_DECLARE_METHOD(LuaSystem, start),
    LUNAR_DECLARE_METHOD(LuaSystem, exit),
    LUNAR_DECLARE_METHOD(LuaSystem, setOptions),
    LUNAR_DECLARE_METHOD(LuaSystem, setTraceMode),
    LUNAR_DECLARE_METHOD(LuaSystem, vclock),
    LUNAR_DECLARE_METHOD(LuaSystem, vsleep),
    LUNAR_DECLARE_METHOD(LuaSystem, sleep),
    {0,0}
};

Lunar<LuaFrontend>::RegType LuaFrontend::methods[] = {
    LUNAR_DECLARE_METHOD(LuaFrontend, init),
    LUNAR_DECLARE_METHOD(LuaFrontend, runFrame),
    LUNAR_DECLARE_METHOD(LuaFrontend, exit),
    LUNAR_DECLARE_METHOD(LuaFrontend, postMessage),    
    {0,0}
};

Lunar<LuaCube>::RegType LuaCube::methods[] = {
    LUNAR_DECLARE_METHOD(LuaCube, reset),
    LUNAR_DECLARE_METHOD(LuaCube, isDebugging),
    LUNAR_DECLARE_METHOD(LuaCube, lcdWriteCount),
    LUNAR_DECLARE_METHOD(LuaCube, lcdPixelCount),
    LUNAR_DECLARE_METHOD(LuaCube, exceptionCount),
    LUNAR_DECLARE_METHOD(LuaCube, getRadioAddress),
    LUNAR_DECLARE_METHOD(LuaCube, handleRadioPacket),
    LUNAR_DECLARE_METHOD(LuaCube, saveScreenshot),
    LUNAR_DECLARE_METHOD(LuaCube, testScreenshot),
    LUNAR_DECLARE_METHOD(LuaCube, testSetEnabled),
    LUNAR_DECLARE_METHOD(LuaCube, testGetACK),
    LUNAR_DECLARE_METHOD(LuaCube, testWriteVRAM),
    LUNAR_DECLARE_METHOD(LuaCube, xbPoke),
    LUNAR_DECLARE_METHOD(LuaCube, xwPoke),
    LUNAR_DECLARE_METHOD(LuaCube, xbPeek),
    LUNAR_DECLARE_METHOD(LuaCube, xwPeek),
    LUNAR_DECLARE_METHOD(LuaCube, ibPoke),
    LUNAR_DECLARE_METHOD(LuaCube, ibPeek),
    LUNAR_DECLARE_METHOD(LuaCube, fwPoke),
    LUNAR_DECLARE_METHOD(LuaCube, fbPoke),
    LUNAR_DECLARE_METHOD(LuaCube, fwPeek),
    LUNAR_DECLARE_METHOD(LuaCube, fbPeek),
    {0,0}
};

Lunar<LuaRuntime>::RegType LuaRuntime::methods[] = {
    LUNAR_DECLARE_METHOD(LuaRuntime, poke),
    {0,0}
};

Lunar<LuaFilesystem>::RegType LuaFilesystem::methods[] = {
    LUNAR_DECLARE_METHOD(LuaFilesystem, newVolume),
    LUNAR_DECLARE_METHOD(LuaFilesystem, listVolumes),
    LUNAR_DECLARE_METHOD(LuaFilesystem, deleteVolume),
    LUNAR_DECLARE_METHOD(LuaFilesystem, volumeType),
    LUNAR_DECLARE_METHOD(LuaFilesystem, volumeMap),
    LUNAR_DECLARE_METHOD(LuaFilesystem, volumeEraseCounts),
    LUNAR_DECLARE_METHOD(LuaFilesystem, simulatedSectorEraseCounts),
    {0,0}
};


LuaScript::LuaScript(System &sys)
{    
    L = lua_open();
    luaL_openlibs(L);

    LuaSystem::sys = &sys;

    Lunar<LuaSystem>::Register(L);
    Lunar<LuaFrontend>::Register(L);
    Lunar<LuaCube>::Register(L);
    Lunar<LuaRuntime>::Register(L);
    Lunar<LuaFilesystem>::Register(L);
}

LuaScript::~LuaScript()
{
    lua_close(L);
}

int LuaScript::runFile(const char *filename)
{
    if (luaL_dofile(L, filename)) {
        fprintf(stderr, "-!- Error: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
        return 1;
    }

    return 0;
}

int LuaScript::runString(const char *str)
{
    if (luaL_dostring(L, str)) {
        fprintf(stderr, "-!- Error: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
        return 1;
    }

    return 0;
}

bool LuaScript::argBegin(lua_State *L, const char *className)
{
    /*
     * Before iterating over table arguments, see if the table is even valid
     */

    if (!lua_istable(L, 1)) {
        luaL_error(L, "%s{} argument is not a table. Did you use () instead of {}?",
                   className);
        return false;
    }

    return true;
}

bool LuaScript::argMatch(lua_State *L, const char *arg)
{
    /*
     * See if we can extract an argument named 'arg' from the table.
     * If we can, returns 'true' with the named argument at the top of
     * the stack. If not, returns 'false'.
     */

    lua_pushstring(L, arg);
    lua_gettable(L, 1);

    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return false;
    }

    lua_pushstring(L, arg);
    lua_pushnil(L);
    lua_settable(L, 1);

    return true;
}
 
bool LuaScript::argMatch(lua_State *L, lua_Integer arg)
{
    lua_pushinteger(L, arg);
    lua_gettable(L, 1);

    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return false;
    }

    lua_pushinteger(L, arg);
    lua_pushnil(L);
    lua_settable(L, 1);

    return true;
}
  
bool LuaScript::argEnd(lua_State *L)
{
    /*
     * Finish iterating over an argument table.
     * If there are any unused arguments, turn them into errors.
     */

    bool success = true;

    lua_pushnil(L);
    while (lua_next(L, 1)) {
        lua_pushvalue(L, -2);   // Make a copy of the key object
        luaL_error(L, "Unrecognized parameter (%s)", lua_tostring(L, -1));
        lua_pop(L, 2);          // Pop value and key-copy.
        success = false;
    }

    return success;
}

LuaSystem::LuaSystem(lua_State *L)
{
    /* Nothing to do; our System object is a singleton provided by main.cpp */
}

int LuaSystem::setOptions(lua_State *L)
{
    if (!LuaScript::argBegin(L, className))
        return 0;
    
    if (LuaScript::argMatch(L, "numCubes"))
        sys->opt_numCubes = lua_tointeger(L, -1);
    
    if (LuaScript::argMatch(L, "cubeFirmware"))
        sys->opt_cubeFirmware = lua_tostring(L, -1);
    
    if (LuaScript::argMatch(L, "turbo"))
        sys->opt_turbo = lua_toboolean(L, -1);
    
    if (LuaScript::argMatch(L, "continueOnException"))
        sys->opt_continueOnException = lua_toboolean(L, -1);

    if (LuaScript::argMatch(L, "cube0Debug"))
        sys->opt_cube0Debug = lua_toboolean(L, -1);

    if (LuaScript::argMatch(L, "cube0Profile"))
        sys->opt_cube0Profile = lua_tostring(L, -1);

    if (!LuaScript::argEnd(L))
        return 0;

    return 0;
}   

int LuaSystem::setTraceMode(lua_State *L)
{
    sys->tracer.setEnabled(lua_toboolean(L, 1));
    return 0;
}

int LuaSystem::vclock(lua_State *L)
{
    /*
     * Read the virtual-time clock, in seconds
     */
    lua_pushnumber(L, sys->time.elapsedSeconds());
    return 1;
}

int LuaSystem::sleep(lua_State *L)
{
    glfwSleep(luaL_checknumber(L, 1));
    return 0;
}
    
int LuaSystem::vsleep(lua_State *L)
{
    /*
     * Sleep, in virtual time
     */

    double deadline = sys->time.elapsedSeconds() + luaL_checknumber(L, 1);
    double remaining;
    
    while ((remaining = deadline - sys->time.elapsedSeconds()) > 0)
        glfwSleep(remaining * 0.1);

    return 0;
}

int LuaSystem::init(lua_State *L)
{
    /*
     * Initialize the system, with current options
     */
    if (!sys->init()) {
        lua_pushfstring(L, "failed to initialize System");
        lua_error(L);
    }
    return 0;
}
 
int LuaSystem::start(lua_State *L)
{
    sys->start();
    return 0;
}

int LuaSystem::exit(lua_State *L)
{
    sys->exit();
    return 0;
}

LuaFrontend::LuaFrontend(lua_State *L) {}

int LuaFrontend::init(lua_State *L)
{
    if (!fe.init(LuaSystem::sys)) {
        lua_pushfstring(L, "failed to initialize Frontend");
        lua_error(L);
    }
    return 0;
}
 
int LuaFrontend::runFrame(lua_State *L)
{
    lua_pushboolean(L, fe.runFrame());
    return 1;
}

int LuaFrontend::exit(lua_State *L)
{
    fe.exit();
    return 0;
}

int LuaFrontend::postMessage(lua_State *L)
{
    fe.postMessage(lua_tostring(L, -1));
    return 0;
}

LuaCube::LuaCube(lua_State *L)
    : id(0)
{
    lua_Integer i = lua_tointeger(L, -1);

    if (i < 0 || i >= (lua_Integer)System::MAX_CUBES) {
        lua_pushfstring(L, "cube ID %d is not valid. Must be between 0 and %d", System::MAX_CUBES);
        lua_error(L);
    } else {
        id = (unsigned) i;
    }
}

int LuaCube::reset(lua_State *L)
{
    LuaSystem::sys->resetCube(id);

    return 0;
}

int LuaCube::isDebugging(lua_State *L)
{
    lua_pushboolean(L, LuaSystem::sys->cubes[id].isDebugging());
    return 1;
}

int LuaCube::lcdWriteCount(lua_State *L)
{
    lua_pushinteger(L, LuaSystem::sys->cubes[id].lcd.getWriteCount());
    return 1;
}

int LuaCube::lcdPixelCount(lua_State *L)
{
    lua_pushinteger(L, LuaSystem::sys->cubes[id].lcd.getPixelCount());
    return 1;
}

int LuaCube::exceptionCount(lua_State *L)
{
    lua_pushinteger(L, LuaSystem::sys->cubes[id].getExceptionCount());
    return 1;
}

int LuaCube::xbPoke(lua_State *L)
{
    uint8_t *mem = &LuaSystem::sys->cubes[id].cpu.mExtData[0];
    mem[(XDATA_SIZE - 1) & luaL_checkinteger(L, 1)] = luaL_checkinteger(L, 2);
    return 0;
}

int LuaCube::xwPoke(lua_State *L)
{
    uint16_t *mem = (uint16_t*) &LuaSystem::sys->cubes[id].cpu.mExtData[0];
    mem[(XDATA_SIZE/2 - 1) & luaL_checkinteger(L, 1)] = luaL_checkinteger(L, 2);
    return 0;
}

int LuaCube::xbPeek(lua_State *L)
{
    uint8_t *mem = &LuaSystem::sys->cubes[id].cpu.mExtData[0];
    lua_pushinteger(L, mem[(XDATA_SIZE - 1) & luaL_checkinteger(L, 1)]);
    return 1;
}

int LuaCube::xwPeek(lua_State *L)
{
    uint16_t *mem = (uint16_t*) &LuaSystem::sys->cubes[id].cpu.mExtData[0];
    lua_pushinteger(L, mem[(XDATA_SIZE/2 - 1) & luaL_checkinteger(L, 1)]);
    return 1;
}
    
int LuaCube::ibPoke(lua_State *L)
{
    uint8_t *mem = &LuaSystem::sys->cubes[id].cpu.mData[0];
    mem[0xFF & luaL_checkinteger(L, 1)] = luaL_checkinteger(L, 2);
    return 0;
}

int LuaCube::ibPeek(lua_State *L)
{
    uint8_t *mem = &LuaSystem::sys->cubes[id].cpu.mData[0];
    lua_pushinteger(L, mem[0xFF & luaL_checkinteger(L, 1)]);
    return 1;
}

int LuaCube::fwPoke(lua_State *L)
{
    uint16_t *mem = (uint16_t*) &LuaSystem::sys->cubes[id].flash.getStorage()->ext;
    mem[(Cube::FlashModel::SIZE/2 - 1) & luaL_checkinteger(L, 1)] = luaL_checkinteger(L, 2);
    return 0;
}

int LuaCube::fbPoke(lua_State *L)
{
    uint8_t *mem = (uint8_t*) &LuaSystem::sys->cubes[id].flash.getStorage()->ext;
    mem[(Cube::FlashModel::SIZE - 1) & luaL_checkinteger(L, 1)] = luaL_checkinteger(L, 2);
    return 0;
}

int LuaCube::fwPeek(lua_State *L)
{
    uint16_t *mem = (uint16_t*) &LuaSystem::sys->cubes[id].flash.getStorage()->ext;
    lua_pushinteger(L, mem[(Cube::FlashModel::SIZE/2 - 1) & luaL_checkinteger(L, 1)]);
    return 1;
}

int LuaCube::fbPeek(lua_State *L)
{
    uint8_t *mem = (uint8_t*) &LuaSystem::sys->cubes[id].flash.getStorage()->ext;
    lua_pushinteger(L, mem[(Cube::FlashModel::SIZE - 1) & luaL_checkinteger(L, 1)]);
    return 1;
}

int LuaCube::saveScreenshot(lua_State *L)
{    
    const char *filename = luaL_checkstring(L, 1);
    Cube::LCD &lcd = LuaSystem::sys->cubes[id].lcd;
    std::vector<uint8_t> pixels;
    
    for (unsigned i = 0; i < lcd.FB_SIZE; i++) {
        RGB565 color = lcd.fb_mem[i];
        pixels.push_back(color.red());
        pixels.push_back(color.green());
        pixels.push_back(color.blue());
        pixels.push_back(0xFF);
    }

    LodePNG::Encoder encoder;
    std::vector<uint8_t> pngData;
    encoder.encode(pngData, pixels, lcd.WIDTH, lcd.HEIGHT);
    
    LodePNG::saveFile(pngData, filename);
    
    return 0;
}

int LuaCube::testScreenshot(lua_State *L)
{
    const char *filename = luaL_checkstring(L, 1);
    Cube::LCD &lcd = LuaSystem::sys->cubes[id].lcd;
    std::vector<uint8_t> pngData;
    std::vector<uint8_t> pixels;
    LodePNG::Decoder decoder;
    
    LodePNG::loadFile(pngData, filename);
    if (!pngData.empty())
        decoder.decode(pixels, pngData);
    
    if (pixels.empty()) {
        lua_pushfstring(L, "error loading PNG file \"%s\"", filename);
        lua_error(L);
    }
    
    for (unsigned i = 0; i < lcd.FB_SIZE; i++) {
        RGB565 fbColor = lcd.fb_mem[i];
        RGB565 refColor = &pixels[i*4];
        
        if (fbColor.value != refColor.value) {
            // Image mismatch. Return (x, y, lcdPixel, refPixel)
            lua_pushinteger(L, i % lcd.WIDTH);
            lua_pushinteger(L, i / lcd.WIDTH);
            lua_pushinteger(L, fbColor.value);
            lua_pushinteger(L, refColor.value);
            return 4;
        }
    }
    
    return 0;
}

int LuaCube::handleRadioPacket(lua_State *L)
{
    /*
     * Argument is a radio packet, represented as a string.
     * Returns a string ACK (which may be empty), or no argument
     * if the packet failed to acknowledge.
     */
     
    Cube::Radio &radio = LuaSystem::sys->cubes[id].spi.radio;    
    
    Cube::Radio::Packet packet, reply;
    memset(&packet, 0, sizeof packet);
    memset(&reply, 0, sizeof reply);

    size_t packetStrLen = 0;
    const char *packetStr = lua_tolstring(L, 1, &packetStrLen);
    
    if (packetStrLen > sizeof packet.payload) {
        lua_pushfstring(L, "packet too long");
        lua_error(L);
        return 0;
    }
    
    packet.len = packetStrLen;
    memcpy(packet.payload, packetStr, packetStrLen);
    
    if (radio.handlePacket(packet, reply)) {
        lua_pushlstring(L, (const char *) reply.payload, reply.len);
        return 1;
    }

    return 0;
}

int LuaCube::getRadioAddress(lua_State *L)
{
    /*
     * Takes no arguments, returns the radio's RX address formatted as a string.
     * Our string format matches the debug logging in the master firmware:
     *
     *    <channel> / <byte0> <byte1> <byte2> <byte3> <byte4>
     */

    Cube::Radio &radio = LuaSystem::sys->cubes[id].spi.radio;   
    char buf[20];
    uint64_t addr = radio.getPackedRXAddr();

    snprintf(buf, sizeof buf, "%02x/%02x%02x%02x%02x%02x",
        (uint8_t)(addr >> 56),
        (uint8_t)(addr >> 0),
        (uint8_t)(addr >> 8),
        (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 24),
        (uint8_t)(addr >> 32));
        
    lua_pushstring(L, buf);
    return 1;
}

int LuaCube::testSetEnabled(lua_State *L)
{
    Cube::I2CTestJig &test = LuaSystem::sys->cubes[id].i2c.testjig;
    test.setEnabled(lua_toboolean(L, 1));
    return 0;
}

int LuaCube::testGetACK(lua_State *L)
{
    Cube::I2CTestJig &test = LuaSystem::sys->cubes[id].i2c.testjig;
    std::vector<uint8_t> buffer;
    test.getACK(buffer);
    lua_pushlstring(L, (const char *) &buffer[0], buffer.size());
    return 1;
}

int LuaCube::testWriteVRAM(lua_State *L)
{
    Cube::I2CTestJig &test = LuaSystem::sys->cubes[id].i2c.testjig;
    test.writeVRAM(luaL_checkinteger(L, 1), luaL_checkinteger(L, 2));
    return 0;
}

LuaRuntime::LuaRuntime(lua_State *L)
{
    /* Nothing to do; the SVM runtime is a singleton */
}

int LuaRuntime::poke(lua_State *L)
{
    SvmMemory::VirtAddr va = luaL_checkinteger(L, 1);
    SvmMemory::PhysAddr pa;
    uint32_t value = luaL_checkinteger(L, 2);

    if (!SvmMemory::mapRAM(va, sizeof value, pa)) {
        lua_pushfstring(L, "invalid RAM address");
        lua_error(L);
        return 0;
    }

    *reinterpret_cast<uint32*>(pa) = value;
    return 0;
}

LuaFilesystem::LuaFilesystem(lua_State *L)
{
    /* Nothing to do; the filesystem is a singleton */
}

int LuaFilesystem::newVolume(lua_State *L)
{
    /*
     * Takes three arguments: (type, payload data, type-specific data).
     * Type-specific data is optional, and will be zero-length if omitted.
     *
     * Creates, writes, and commits the new volume. Returns its block code
     * on success, or nil on failure.
     */

    size_t payloadStrLen = 0;
    size_t dataStrLen = 0;
    unsigned type = luaL_checkinteger(L, 1);
    const char *payloadStr = lua_tolstring(L, 2, &payloadStrLen);
    const char *dataStr = lua_tolstring(L, 3, &dataStrLen);

    FlashVolumeWriter writer;
    if (!writer.begin(type, payloadStrLen, dataStrLen))
        return 0;

    writer.appendPayload((const uint8_t*)payloadStr, payloadStrLen);
    writer.commit();

    lua_pushinteger(L, writer.volume.block.code);
    return 1;
}

int LuaFilesystem::listVolumes(lua_State *L)
{
    /*
     * Takes no arguments. Returns an array of volume block codes.
     */

    lua_newtable(L);

    FlashVolumeIter vi;
    FlashVolume vol;
    unsigned index = 0;

    while (vi.next(vol)) {
        lua_pushnumber(L, ++index);
        lua_pushnumber(L, vol.block.code);
        lua_settable(L, -3);
    }
    
    return 1;
}

int LuaFilesystem::deleteVolume(lua_State *L)
{
    /*
     * Given a volume block code, mark the volume as deleted.
     */

    unsigned code = luaL_checkinteger(L, 1);
    FlashVolume vol(FlashMapBlock::fromCode(code));
    if (!vol.isValid()) {
        lua_pushfstring(L, "invalid volume");
        lua_error(L);
        return 0;
    }

    vol.markAsDeleted();
    return 0;
}

int LuaFilesystem::volumeType(lua_State *L)
{
    /*
     * Given a volume block code, return the volume's type.
     */

    unsigned code = luaL_checkinteger(L, 1);
    FlashVolume vol(FlashMapBlock::fromCode(code));
    if (!vol.isValid()) {
        lua_pushfstring(L, "invalid volume");
        lua_error(L);
        return 0;
    }

    lua_pushnumber(L, vol.getType());
    return 1;
}

int LuaFilesystem::volumeMap(lua_State *L)
{
    /*
     * Given a volume block code, return the volume's map
     * as an array of block codes.
     */

    unsigned code = luaL_checkinteger(L, 1);
    FlashVolume vol(FlashMapBlock::fromCode(code));
    if (!vol.isValid()) {
        lua_pushfstring(L, "invalid volume");
        lua_error(L);
        return 0;
    }

    FlashBlockRef ref;
    FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, vol.block);
    const FlashMap *map = hdr->getMap();

    lua_newtable(L);

    for (unsigned I = 0, E = hdr->numMapEntries(); I != E; ++I) {
        lua_pushnumber(L, I + 1);
        lua_pushnumber(L, map->blocks[I].code);
        lua_settable(L, -3);
    }

    return 1;
}

int LuaFilesystem::volumeEraseCounts(lua_State *L)
{
    /*
     * Given a volume block code, return the volume's array of erase counts.
     */

    unsigned code = luaL_checkinteger(L, 1);
    FlashVolume vol(FlashMapBlock::fromCode(code));
    if (!vol.isValid()) {
        lua_pushfstring(L, "invalid volume");
        lua_error(L);
        return 0;
    }

    FlashBlockRef hdrRef, eraseRef;
    FlashVolumeHeader *hdr = FlashVolumeHeader::get(hdrRef, vol.block);

    lua_newtable(L);

    for (unsigned I = 0, E = hdr->numMapEntries(); I != E; ++I) {
        lua_pushnumber(L, I + 1);
        lua_pushnumber(L, hdr->getEraseCount(eraseRef, vol.block, I));
        lua_settable(L, -3);
    }

    return 1;
}

int LuaFilesystem::simulatedSectorEraseCounts(lua_State *L)
{
    /*
     * No parameters. Returns a table of simulated erase counts for the
     * master flash memory, one per sector.
     */

    FlashStorage::MasterRecord &storage = SystemMC::getSystem()->flash.data->master;

    lua_newtable(L);

    for (unsigned i = 0; i != arraysize(storage.eraseCounts); ++i) {
        lua_pushnumber(L, i + 1);
        lua_pushnumber(L, storage.eraseCounts[i]);
        lua_settable(L, -3);
    }

    return 1;
}