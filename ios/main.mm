#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include "../core/forge.hpp"

@interface ForgeViewController : UIViewController <UIDocumentPickerDelegate>
@property(nonatomic,strong) UITextView *output;
@property(nonatomic,strong) UIButton *demoButton;
@property(nonatomic,strong) UIButton *openButton;
@property(nonatomic,strong) UIButton *shareButton;
@end
@implementation ForgeViewController
- (UIButton *)button:(NSString *)title action:(SEL)action {
    UIButton *b=[UIButton buttonWithType:UIButtonTypeSystem];
    b.configuration=[UIButtonConfiguration filledButtonConfiguration];
    [b setTitle:title forState:UIControlStateNormal];[b addTarget:self action:action forControlEvents:UIControlEventTouchUpInside];return b;
}
- (void)viewDidLoad {
    [super viewDidLoad];self.view.backgroundColor=UIColor.systemBackgroundColor;
    UILabel *title=[UILabel new];title.text=@"ForgeWin · P2";title.font=[UIFont boldSystemFontOfSize:30];
    UILabel *subtitle=[UILabel new];subtitle.text=@"Experimental Windows execution core\niPhone · ARM64 · iOS 16+";subtitle.numberOfLines=0;subtitle.textColor=UIColor.secondaryLabelColor;
    self.demoButton=[self button:@"Run compiled Windows C test" action:@selector(runDemo)];
    self.openButton=[self button:@"Open .exe for diagnostics" action:@selector(openFile)];
    self.shareButton=[self button:@"Share diagnostic log" action:@selector(shareLog)];
    self.output=[UITextView new];self.output.editable=NO;self.output.font=[UIFont monospacedSystemFontOfSize:12 weight:UIFontWeightRegular];
    self.output.text=@"P2: a Windows x64 program compiled from C with Microsoft Visual C.\n\nThe same EXE is tested on Windows and in ForgeWin during the build. It computes a weighted array sum through a C function, prints a message and exits.\n\nExpected: ExitProcess code=251\nForgeWin P2: compiled C program OK\n\nThis remains a limited interpreter. Commercial games, graphics, audio and JIT are not supported yet.";
    UIStackView *stack=[[UIStackView alloc] initWithArrangedSubviews:@[title,subtitle,self.demoButton,self.openButton,self.shareButton,self.output]];
    stack.axis=UILayoutConstraintAxisVertical;stack.spacing=12;stack.translatesAutoresizingMaskIntoConstraints=NO;[self.view addSubview:stack];
    UILayoutGuide *safe=self.view.safeAreaLayoutGuide;
    [NSLayoutConstraint activateConstraints:@[[stack.topAnchor constraintEqualToAnchor:safe.topAnchor constant:16],[stack.leadingAnchor constraintEqualToAnchor:safe.leadingAnchor constant:16],[stack.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor constant:-16],[stack.bottomAnchor constraintEqualToAnchor:safe.bottomAnchor constant:-12]]];
}
- (void)runDemo { [self executeURL:[[NSBundle mainBundle] URLForResource:@"compiled251" withExtension:@"exe"]]; }
- (void)openFile {
    UIDocumentPickerViewController *picker=[[UIDocumentPickerViewController alloc] initForOpeningContentTypes:@[UTTypeData] asCopy:YES];picker.delegate=self;[self presentViewController:picker animated:YES completion:nil];
}
- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls {if(urls.count)[self executeURL:urls.firstObject];}
- (void)executeURL:(NSURL *)url {
    if(!url){self.output.text=@"Missing bundled executable.";return;}
    self.demoButton.enabled=NO;self.openButton.enabled=NO;self.shareButton.enabled=NO;self.output.text=@"Running guest instructions…";
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED,0), ^{
        @autoreleasepool {
            NSString *report=nil;NSNumber *size=nil;NSError *error=nil;
            [url getResourceValue:&size forKey:NSURLFileSizeKey error:&error];
            if(!size||size.unsignedLongLongValue>64ull*1024*1024){report=@"Cannot read file, or file exceeds P1's 64 MiB limit.";}
            else {
                NSData *data=[NSData dataWithContentsOfURL:url options:NSDataReadingMappedIfSafe error:&error];
                if(!data){report=[NSString stringWithFormat:@"Read failed: %@",error.localizedDescription];}
                else {
                    try {
                        forge::Bytes bytes(data.length);if(data.length)memcpy(bytes.data(),data.bytes,data.length);
                        std::string log=forge::execute(bytes);report=[NSString stringWithUTF8String:log.c_str()];
                    }catch(const std::exception& e){report=[NSString stringWithFormat:@"Host error: %s",e.what()];}
                }
            }
            NSString *full=[NSString stringWithFormat:@"Device: %@\niOS: %@\nFile: %@\n%@",UIDevice.currentDevice.model,UIDevice.currentDevice.systemVersion,url.lastPathComponent,report];
            dispatch_async(dispatch_get_main_queue(), ^{self.output.text=full;self.demoButton.enabled=YES;self.openButton.enabled=YES;self.shareButton.enabled=YES;});
        }
    });
}
- (void)shareLog {
    NSURL *url=[[NSURL fileURLWithPath:NSTemporaryDirectory()] URLByAppendingPathComponent:@"ForgeWin-diagnostic.txt"];
    NSError *error=nil;if(![self.output.text writeToURL:url atomically:YES encoding:NSUTF8StringEncoding error:&error]){self.output.text=error.localizedDescription;return;}
    UIActivityViewController *share=[[UIActivityViewController alloc] initWithActivityItems:@[url] applicationActivities:nil];
    share.popoverPresentationController.sourceView=self.shareButton;share.popoverPresentationController.sourceRect=self.shareButton.bounds;[self presentViewController:share animated:YES completion:nil];
}
@end
@interface ForgeApp : UIResponder <UIApplicationDelegate>
@property(nonatomic,strong) UIWindow *window;
@end
@implementation ForgeApp
- (BOOL)application:(UIApplication *)app didFinishLaunchingWithOptions:(NSDictionary *)options {
    self.window=[[UIWindow alloc]initWithFrame:UIScreen.mainScreen.bounds];self.window.rootViewController=[ForgeViewController new];[self.window makeKeyAndVisible];return YES;
}
@end
int main(int argc,char *argv[]){@autoreleasepool{return UIApplicationMain(argc,argv,nil,NSStringFromClass(ForgeApp.class));}}
