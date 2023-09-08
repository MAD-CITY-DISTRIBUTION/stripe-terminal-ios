//
//  SetupIntentViewController.swift
//  AppStore
//
//  Created by James Little on 1/13/21.
//  Copyright © 2021 Stripe. All rights reserved.
//

import UIKit
import Static
import StripeTerminal

class SetupIntentViewController: EventDisplayingViewController {

    override var cancelLogMethod: LogEvent.Method {
        return .cancelCollectSetupIntentPaymentMethod
    }

    override func viewDidLoad() {
        super.viewDidLoad()

        do {
            let params = try SetupIntentParametersBuilder().build()

            createSetupIntent(params) { intent, createError in
                if createError != nil {
                    self.complete()
                } else if let intent = intent {
                    // 2. collectSetupIntent
                    self.collectSetupIntent(intent: intent)
                }
            }
        } catch {
            self.presentAlert(error: error)
        }
    }

    private func createSetupIntent(_ params: SetupIntentParameters, completion: @escaping SetupIntentCompletionBlock) {
        var createEvent = LogEvent(method: .createSetupIntent)
        self.events.append(createEvent)
        Terminal.shared.createSetupIntent(params, completion: { (createdSetupIntent, createError) in
            if let error = createError {
                createEvent.result = .errored
                createEvent.object = .error(error as NSError)
                self.events.append(createEvent)
            } else if let si = createdSetupIntent {
                createEvent.result = .succeeded
                createEvent.object = .setupIntent(si)
                self.events.append(createEvent)
            }
            completion(createdSetupIntent, createError)
        })
    }

    private func collectSetupIntent(intent: SetupIntent) {
        var collectEvent = LogEvent(method: .collectSetupIntentPaymentMethod)
        self.events.append(collectEvent)
        self.cancelable = Terminal.shared.collectSetupIntentPaymentMethod(intent, customerConsentCollected: true) { (collectedSetupIntent, collectError) in
            if let error = collectError {
                collectEvent.result = .errored
                collectEvent.object = .error(error as NSError)
                self.events.append(collectEvent)
                if (error as NSError).code == ErrorCode.canceled.rawValue {
                    self.cancelSetupIntent(intent: intent)
                } else {
                    self.complete()
                }
            } else if let intent = collectedSetupIntent {
                collectEvent.result = .succeeded
                collectEvent.object = .setupIntent(intent)
                self.events.append(collectEvent)
                self.confirmSetupIntent(intent)
            }
        }
    }

    private func cancelSetupIntent(intent: SetupIntent) {
        var cancelEvent = LogEvent(method: .cancelSetupIntent)
        self.events.append(cancelEvent)
        Terminal.shared.cancelSetupIntent(intent) { canceledSetupIntent, cancelError in
            if let error = cancelError {
                cancelEvent.result = .errored
                cancelEvent.object = .error(error as NSError)
                self.events.append(cancelEvent)
            } else if let intent = canceledSetupIntent {
                cancelEvent.result = .succeeded
                cancelEvent.object = .setupIntent(intent)
                self.events.append(cancelEvent)
            }
            self.complete()
        }
    }

    private func confirmSetupIntent(_ intent: SetupIntent) {
        var processEvent = LogEvent(method: .confirmSetupIntent)
        self.events.append(processEvent)
        Terminal.shared.confirmSetupIntent(intent) { processedIntent, processError in
            if let error = processError {
                processEvent.result = .errored
                processEvent.object = .error(error as NSError)
                self.events.append(processEvent)
                self.complete()
            } else if let intent = processedIntent {
                if intent.status == .succeeded {
                    processEvent.result = .succeeded
                    processEvent.object = .setupIntent(intent)
                    self.events.append(processEvent)
                    self.complete()
                } else {
                    processEvent.result = .errored
                    processEvent.object = .setupIntent(intent)
                    self.events.append(processEvent)
                    self.complete()
                }
            }
        }
    }
}
