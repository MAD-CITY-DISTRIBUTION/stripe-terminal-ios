//
//  SCPTerminal.h
//  StripeTerminal
//
//  Created by Ben Guo on 7/28/17.
//  Copyright © 2017 Stripe. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "SCPBlocks.h"
#import "SCPConnectionStatus.h"
#import "SCPDeviceType.h"
#import "SCPPaymentStatus.h"
#import "SCPReaderInputDelegate.h"

NS_ASSUME_NONNULL_BEGIN

/**
 The current version of this library.
 */
static NSString *const SCPSDKVersion = @"1.0-b1";

@class SCPCancelable, SCPDiscoveryConfiguration, SCPTerminalConfiguration, SCPPaymentIntentParameters, SCPReadSourceParameters;

@protocol SCPConnectionTokenProvider, SCPTerminalDelegate, SCPDiscoveryDelegate, SCPUpdateReaderDelegate;

/**
 The Terminal object that is made available by the Stripe Terminal SDK exposes
 a generic interface for discovering readers, connecting to a reader, and
 creating payments.

 Note: You should only initialize a Terminal once in your app. Behavior is
 undefined if you create multiple Terminal instances.
 */
NS_SWIFT_NAME(Terminal)
@interface SCPTerminal : NSObject

/**
 The terminal's delegate.
 */
@property (nonatomic, weak, nullable, readwrite) id<SCPTerminalDelegate> terminalDelegate;

/**
 The configuration used to initialize the terminal.
 */
@property (nonatomic, readonly) SCPTerminalConfiguration *configuration;

/**
 Information about the connected reader, or nil if no reader is connected.
 */
@property (nonatomic, nullable, readonly) SCPReader *connectedReader;

/**
 The terminal's current connection status.
 */
@property (nonatomic, readonly) SCPConnectionStatus connectionStatus;

/**
 The terminal's current payment status.
 */
@property (nonatomic, readonly) SCPPaymentStatus paymentStatus;

/**
 Initializes a terminal with the given configuration, connection token
 provider, and delegate.

 @param configuration           The configuration for the terminal.
 @param tokenProvider           Your connection token provider.
 @param delegate                The terminal's delegate.
 */
- (instancetype)initWithConfiguration:(SCPTerminalConfiguration *)configuration
                        tokenProvider:(id<SCPConnectionTokenProvider>)tokenProvider
                             delegate:(id<SCPTerminalDelegate>)delegate;

/**
 Clears the current connection token. You can use this method to switch
 accounts in your app, e.g. to switch between live and test Stripe API keys on
 your backend.
 
 In order to switch accounts in your app:
 - if a reader is connected, call `disconnect`
 - call `clearConnectionToken`
 - call `discover` and `connect` to connect to a reader. The `connect` call will
 request a new connection token from your backend server.
 
 An overview of the lifecycle of a connection token under the hood:
 - When a Terminal is initialized, the SDK attempts to proactively request
 a connection token from your backend server.
 - When `connect` is called, the SDK uses the connection token and reader
 information to create a reader session.
 - Subsequent calls to `connect` require a new connection token. If you
 disconnect from a reader, and then call `connect` again, the SDK will fetch
 another connection token.
 */
- (void)clearConnectionToken;

/**
 Begins discovering readers matching the given configuration.

 When `discover` is called, the terminal begins scanning for readers using
 the settings in the given DiscoveryConfiguration. You must implement
 `DiscoveryDelegate` to handle displaying discovery results to your user and
 connecting to a selected reader.

 The discovery process will stop on its own when the terminal successfully
 connects to a reader, if the command is canceled, or if an error occurs.

 To end discovery after a specified time interval, set the `timeout` property
 on your DiscoveryConfiguration.

 Be sure to either set a timeout, or make it possible to cancel `discover` in
 your app's UI.
 
 @param configuration   The configuration for reader discovery.
 @param delegate        Your delegate for reader discovery.
 @param completion      The completion block called when the command completes.
 */
- (nullable SCPCancelable *)discoverReaders:(SCPDiscoveryConfiguration *)configuration
                                   delegate:(id<SCPDiscoveryDelegate>)delegate
                                 completion:(SCPErrorCompletionBlock)completion;

/**
 Attempts to connect to the given reader.

 If the connect succeeds, the completion block will be called with the
 connected reader, and the terminal's `connectionStatus` will change to
 `Connected`.

 If the connect fails, the completion block will be called with an error.

 Under the hood, the SDK uses the `fetchConnectionToken` method you defined
 to fetch a connection token if it does not already have one. It then uses the
 connection token and reader information to create a reader session.

 @param reader          The reader to connect to. This should be a reader
 recently returned to the `didUpdateDiscoveredReaders:` method.
 @param completion      The completion block called when the command completes.
 */
- (void)connectReader:(SCPReader *)reader completion:(SCPReaderCompletionBlock)completion NS_SWIFT_NAME(connectReader(_:completion:));

/**
 Attempts to disconnect from the currently connected reader.
 
 If the disconnect succeeds, the completion block is called with nil. If the
 disconnect fails, the completion block is called with an error.
 
 @param completion      The completion block called when the command completes.
 */
- (void)disconnectReader:(SCPErrorCompletionBlock)completion;

/**
 Creates a new PaymentIntent with the given parameters.
 
 Note: If the information required to create a PaymentIntent isn't readily
 available in your app, you can create the PaymentIntent on your server and use
 the `retrievePaymentIntent` method to retrieve the PaymentIntent in your app.
 
 @param parameters      The parameters for the PaymentIntent to be created.
 @param completion      The completion block called when the command completes.
 */
- (void)createPaymentIntent:(SCPPaymentIntentParameters *)parameters
                 completion:(SCPPaymentIntentCompletionBlock)completion;

/**
 Retrieves a PaymentIntent with a client secret.
 
 If the information required to create a PaymentIntent isn't readily available
 in your app, you can create the PaymentIntent on your server and use this
 method to retrieve the PaymentIntent in your app.
 
 @see https://stripe.com/docs/api#retrieve_payment_intent
 
 @param clientSecret    The client secret of the PaymentIntent to be retrieved.
 @param completion      The completion block called when the command completes.
 */
- (void)retrievePaymentIntent:(NSString *)clientSecret
                   completion:(SCPPaymentIntentCompletionBlock)completion;

/**
 Collects a payment method for the given PaymentIntent.

 If collecting a payment method fails, the completion block will be called with
 an error. After resolving the error, you may call collectPaymentMethod again to either
 try the same card again, or try a different card.

 If collecting a payment method succeeds, the completion block will be called
 with a PaymentIntent with status RequiresConfirmation, indicating that you
 should call confirmPaymentIntent to finish the payment.
 
 @param paymentIntent       The PaymentIntent to collect a payment method for.
 @param delegate            Your delegate for handling reader input events.
 @param completion          The completion block called when the command completes.
 */
- (nullable SCPCancelable *)collectPaymentMethod:(SCPPaymentIntent *)paymentIntent
                                        delegate:(id<SCPReaderInputDelegate>)delegate
                                      completion:(SCPPaymentIntentCompletionBlock)completion;

/**
 Confirms a PaymentIntent. Call this immediately after receiving a
 PaymentIntent from collectPaymentMethod.
 
 When confirming a PaymentIntent fails, the SDK returns an error that includes
 the updated PaymentIntent. Your app should inspect the updated PaymentIntent
 to decide how to retry the payment.

 If the updated PaymentIntent is `nil`, the request to Stripe's servers timed
 out and the PaymentIntent's status is unknown. We recommend that you retry
 confirming the original PaymentIntent. If you instead choose to abandon the
 original PaymentIntent and create a new one, do not to capture the original
 PaymentIntent. If you do, you might charge your customer twice.

 If the updated PaymentIntent's status is still `requires_confirmation` (e.g.,
 the request failed because your app is not connected to the internet), you
 can call `confirmPaymentIntent` again with the updated PaymentIntent to retry
 the request.

 If the updated PaymentIntent's status changes to `requires_source` (e.g., the
 request failed because the card was declined), call `collectPaymentMethod` with the
 updated PaymentIntent to try charging another card.

 If confirming the PaymentIntent succeeds, the completion block will be called
 with a PaymentIntent object with status RequiresCapture,

 Stripe Terminal uses two-step card payments to prevent unintended and duplicate
 payments. When the SDK returns a confirmed PaymentIntent to your app, a charge
 has been authorized but not yet settled, or captured. On your backend, capture
 the confirmed PaymentIntent.

 @param paymentIntent   The PaymentIntent confirm.
 @param completion      The completion block called when the confirm completes.
 */
- (void)confirmPaymentIntent:(SCPPaymentIntent *)paymentIntent
                  completion:(SCPConfirmPaymentIntentCompletionBlock)completion;

/**
 Cancels a PaymentIntent.
 
 If the cancel request succeeds, the completion block will be called with the
 updated PaymentIntent object with status Canceled. If the cancel request
 fails, the completion block will be called with an error.
 
 @param paymentIntent     The PaymentIntent to cancel.
 @param completion        The completion block called when the cancel completes.
 */
- (void)cancelPaymentIntent:(SCPPaymentIntent *)paymentIntent
                 completion:(SCPPaymentIntentCompletionBlock)completion;

/**
 Reads a payment method with the given parameters and returns a Stripe source.

 Note that sources created using this method cannot be charged. Use
 `collectPaymentMethod` and `confirmPaymentIntent` if you are collecting a
 payment from a customer. Use this method to read payment details without
 charging the customer.

 If reading a source fails, the completion block will be called with an error
 containing details about the failure. If reading a source succeeds, the
 completion block will be called with a `CardPresentSource`. You should send
 the ID of the source to your backend for further processing. For example,
 you can use source's fingerprint to look up a charge created using the same
 card.
 
 @param parameters  The parameters for reading the source.
 @param delegate    Your delegate for handling reader input events.
 @param completion  The completion block called when the command completes.
 */
- (nullable SCPCancelable *)readSource:(SCPReadSourceParameters *)parameters
                              delegate:(id<SCPReaderInputDelegate>)delegate
                            completion:(SCPCardPresentSourceCompletionBlock)completion;

/**
 Checks for a reader update and prompts your app to begin installing the update.
 
 If an update is available, the completion block will be called with nil.
 the delegate's  `readerUpdateAvailable:` method will be called, and you will
 have the opportunity to either begin or cancel the update.

 If no update is available, or an error occurs checking for an update, the
 completion block will be called with an error.
 
 @param delegate    Your delegate for handling update events.
 @param completion  The completion block called when checking for an update
 completes.
 */
- (void)updateReader:(id<SCPUpdateReaderDelegate>)delegate
          completion:(SCPErrorCompletionBlock)completion;

/**
 Note: you must first install the Stripe iOS SDK to use this method.
 @see https://stripe.com/docs/mobile/ios#getting-started
 
 Creates a card source using the contents of STPPaymentCardTextField. If the
 field is not valid, the completion block is called with an error.
 
 @param paymentCardTextField    The STPPaymentCardTextField in which a user has
 entered card details.
 
 @param completion  The completion block called when the command completes.
 */
- (void)createKeyedSource:(id)paymentCardTextField
               completion:(SCPCardSourceCompletionBlock)completion;

/**
 Returns an unlocalized string for the given reader input options, e.g.
 "Swipe / Insert"
 */
+ (NSString *)stringFromReaderInputOptions:(SCPReaderInputOptions)options;

/**
 Returns an unlocalized string for the given reader input prompt, e.g.
 "Retry Card"
 */
+ (NSString *)stringFromReaderInputPrompt:(SCPReaderInputPrompt)prompt;

/**
 Returns an unlocalized string for the given connection status, e.g.
 "Connecting"
 */
+ (NSString *)stringFromConnectionStatus:(SCPConnectionStatus)state;

/**
 Returns an unlocalized string for the given payment status, e.g.
 "Not Ready"
 */
+ (NSString *)stringFromPaymentStatus:(SCPPaymentStatus)state;

/**
 Returns an unlocalized string for the given device type.
 */
+ (NSString *)stringFromDeviceType:(SCPDeviceType)deviceType;

/**
 Use `initWithConfiguration:tokenProvider:delegate:`
 */
- (instancetype)init NS_UNAVAILABLE;

/**
 Use `initWithConfiguration:tokenProvider:delegate:`
 */
- (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END