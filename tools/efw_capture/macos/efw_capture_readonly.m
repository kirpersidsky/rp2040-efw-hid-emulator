// Read-only ZWO EFW metadata and HID-descriptor collector for macOS.
// This source intentionally does not open HID devices or exchange reports.

#import <Foundation/Foundation.h>
#import <IOKit/hid/IOHIDKeys.h>
#import <IOKit/hid/IOHIDManager.h>
#import <CommonCrypto/CommonDigest.h>
#import <sys/utsname.h>
#import <unistd.h>
#import <errno.h>

static const uint32_t kTargetVendorID = 0x03C3;
static const uint32_t kTargetProductID = 0x1F01;
static NSString *const kCollectorVersion = @"0.1.0";

static void printUsage(FILE *stream) {
    fprintf(stream, "Usage: efw_capture_readonly --output <path> [--force]\n");
}

static void fail(NSString *message) {
    fprintf(stderr, "error: %s\n", message.UTF8String);
    exit(EXIT_FAILURE);
}

static NSString *hexUpper(const uint8_t *bytes, NSUInteger length) {
    NSMutableString *result = [NSMutableString stringWithCapacity:length * 3];
    for (NSUInteger index = 0; index < length; ++index) {
        if (index != 0) {
            [result appendString:@" "];
        }
        [result appendFormat:@"%02X", bytes[index]];
    }
    return result;
}

static NSString *sha256Hex(NSData *data) {
    unsigned char digest[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256(data.bytes, (CC_LONG)data.length, digest);
    NSMutableString *result = [NSMutableString stringWithCapacity:CC_SHA256_DIGEST_LENGTH * 2];
    for (NSUInteger index = 0; index < sizeof(digest); ++index) {
        [result appendFormat:@"%02x", digest[index]];
    }
    return result;
}

static NSNumber *numberProperty(IOHIDDeviceRef device, CFStringRef key) {
    CFTypeRef value = IOHIDDeviceGetProperty(device, key);
    if (value == NULL || CFGetTypeID(value) != CFNumberGetTypeID()) {
        return nil;
    }
    return (__bridge NSNumber *)value;
}

static NSString *stringProperty(IOHIDDeviceRef device, CFStringRef key) {
    CFTypeRef value = IOHIDDeviceGetProperty(device, key);
    if (value == NULL || CFGetTypeID(value) != CFStringGetTypeID()) {
        return nil;
    }
    return (__bridge NSString *)value;
}

static BOOL writeAtomicallyWithoutOverwrite(NSData *data, NSString *outputPath,
                                            BOOL force, NSError **outError) {
    NSFileManager *manager = [NSFileManager defaultManager];
    NSString *directory = [outputPath stringByDeletingLastPathComponent];
    if (directory.length == 0) {
        directory = @".";
    }
    BOOL isDirectory = NO;
    if (![manager fileExistsAtPath:directory isDirectory:&isDirectory] || !isDirectory) {
        if (outError != NULL) {
            *outError = [NSError errorWithDomain:@"EFWCapture" code:1
                                         userInfo:@{NSLocalizedDescriptionKey: @"output directory does not exist"}];
        }
        return NO;
    }
    if ([manager fileExistsAtPath:outputPath]) {
        if (!force) {
            if (outError != NULL) {
                *outError = [NSError errorWithDomain:@"EFWCapture" code:EEXIST
                                             userInfo:@{NSLocalizedDescriptionKey: @"output file already exists (use --force to replace it)"}];
            }
            return NO;
        }
        if (![manager removeItemAtPath:outputPath error:outError]) {
            return NO;
        }
    }

    NSString *temporaryPath = [directory stringByAppendingPathComponent:
        [NSString stringWithFormat:@".%@.%@.tmp", outputPath.lastPathComponent,
                                   [NSUUID UUID].UUIDString]];
    if (![data writeToFile:temporaryPath options:0 error:outError]) {
        return NO;
    }

    // link(2) creates the final name atomically and fails if another writer won
    // the race. The temporary file is in the same directory, hence same volume.
    if (link(temporaryPath.fileSystemRepresentation, outputPath.fileSystemRepresentation) != 0) {
        int linkError = errno;
        [manager removeItemAtPath:temporaryPath error:nil];
        if (outError != NULL) {
            *outError = [NSError errorWithDomain:NSPOSIXErrorDomain code:linkError userInfo:nil];
        }
        return NO;
    }
    if (unlink(temporaryPath.fileSystemRepresentation) != 0) {
        int unlinkError = errno;
        if (outError != NULL) {
            *outError = [NSError errorWithDomain:NSPOSIXErrorDomain code:unlinkError userInfo:nil];
        }
        return NO;
    }
    return YES;
}

int main(int argc, const char *argv[]) {
    @autoreleasepool {
        NSString *outputPath = nil;
        BOOL force = NO;
        for (int index = 1; index < argc; ++index) {
            NSString *argument = [NSString stringWithUTF8String:argv[index]];
            if ([argument isEqualToString:@"--help"] || [argument isEqualToString:@"-h"]) {
                printUsage(stdout);
                return EXIT_SUCCESS;
            }
            if ([argument isEqualToString:@"--force"]) {
                if (force) {
                    printUsage(stderr);
                    return EXIT_FAILURE;
                }
                force = YES;
                continue;
            }
            if ([argument isEqualToString:@"--output"] && index + 1 < argc && outputPath == nil) {
                outputPath = [NSString stringWithUTF8String:argv[++index]];
                continue;
            }
            printUsage(stderr);
            return EXIT_FAILURE;
        }
        if (outputPath == nil || outputPath.length == 0) {
            printUsage(stderr);
            return EXIT_FAILURE;
        }

        NSDictionary *matching = @{
            @kIOHIDVendorIDKey: @(kTargetVendorID),
            @kIOHIDProductIDKey: @(kTargetProductID),
        };
        IOHIDManagerRef manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
        if (manager == NULL) {
            fail(@"IOHIDManagerCreate failed");
        }
        IOHIDManagerSetDeviceMatching(manager, (__bridge CFDictionaryRef)matching);
        CFSetRef devices = IOHIDManagerCopyDevices(manager);
        CFIndex count = devices == NULL ? 0 : CFSetGetCount(devices);
        if (count != 1) {
            if (devices != NULL) {
                CFRelease(devices);
            }
            CFRelease(manager);
            fail([NSString stringWithFormat:@"expected exactly one ZWO EFW HID device (%04X:%04X), found %ld",
                  kTargetVendorID, kTargetProductID, (long)count]);
        }

        const void *values[1];
        CFSetGetValues(devices, values);
        IOHIDDeviceRef device = (IOHIDDeviceRef)values[0];
        NSNumber *vendorID = numberProperty(device, CFSTR(kIOHIDVendorIDKey));
        NSNumber *productID = numberProperty(device, CFSTR(kIOHIDProductIDKey));
        if (vendorID == nil || productID == nil || vendorID.unsignedIntValue != kTargetVendorID ||
            productID.unsignedIntValue != kTargetProductID) {
            CFRelease(devices);
            CFRelease(manager);
            fail(@"matched HID device did not retain the required VID/PID properties");
        }
        CFTypeRef descriptorValue = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDReportDescriptorKey));
        if (descriptorValue == NULL || CFGetTypeID(descriptorValue) != CFDataGetTypeID()) {
            CFRelease(devices);
            CFRelease(manager);
            fail(@"kIOHIDReportDescriptorKey is missing or is not CFData");
        }
        NSData *descriptor = (__bridge NSData *)descriptorValue;

        NSMutableDictionary *deviceJSON = [@{
            @"vid": [NSString stringWithFormat:@"%04X", vendorID.unsignedIntValue],
            @"pid": [NSString stringWithFormat:@"%04X", productID.unsignedIntValue],
        } mutableCopy];
        NSString *manufacturer = stringProperty(device, CFSTR(kIOHIDManufacturerKey));
        NSString *product = stringProperty(device, CFSTR(kIOHIDProductKey));
        NSString *transport = stringProperty(device, CFSTR(kIOHIDTransportKey));
        if (manufacturer != nil) { deviceJSON[@"manufacturer"] = manufacturer; }
        if (product != nil) { deviceJSON[@"product"] = product; }
        if (transport != nil) { deviceJSON[@"transport"] = transport; }

        struct utsname systemInfo;
        if (uname(&systemInfo) != 0) {
            CFRelease(devices);
            CFRelease(manager);
            fail(@"uname failed");
        }
        NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
        formatter.locale = [[NSLocale alloc] initWithLocaleIdentifier:@"en_US_POSIX"];
        formatter.timeZone = [NSTimeZone timeZoneForSecondsFromGMT:0];
        formatter.dateFormat = @"yyyy-MM-dd'T'HH:mm:ss'Z'";

        NSDictionary *capture = @{
            @"schema_version": @"1.0.0",
            @"collector_version": kCollectorVersion,
            @"captured_at_utc": [formatter stringFromDate:[NSDate date]],
            @"platform": @{
                @"os": @"macOS",
                @"os_version": [[NSProcessInfo processInfo] operatingSystemVersionString],
                @"cpu_architecture": [NSString stringWithUTF8String:systemInfo.machine],
            },
            @"hid_api": @"IOKit / IOHIDManager (metadata and report descriptor only)",
            @"device": deviceJSON,
            @"hid_descriptor": @{
                @"length": @(descriptor.length),
                @"sha256": sha256Hex(descriptor),
                @"raw_bytes_hex": hexUpper(descriptor.bytes, descriptor.length),
            },
            @"operations": @[],
        };
        NSError *jsonError = nil;
        NSData *jsonData = [NSJSONSerialization dataWithJSONObject:capture
                                                            options:NSJSONWritingPrettyPrinted
                                                              error:&jsonError];
        if (jsonData == nil) {
            CFRelease(devices);
            CFRelease(manager);
            fail([NSString stringWithFormat:@"JSON serialization failed: %@", jsonError]);
        }
        NSError *writeError = nil;
        if (!writeAtomicallyWithoutOverwrite(jsonData, outputPath, force, &writeError)) {
            CFRelease(devices);
            CFRelease(manager);
            fail([NSString stringWithFormat:@"could not write output JSON: %@", writeError]);
        }
        printf("saved read-only EFW metadata and descriptor capture to %s\n", outputPath.fileSystemRepresentation);
        CFRelease(devices);
        CFRelease(manager);
    }
    return EXIT_SUCCESS;
}
