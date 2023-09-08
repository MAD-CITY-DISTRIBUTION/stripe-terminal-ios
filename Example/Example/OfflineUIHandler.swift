//
//  OfflineUIHandler.swift
//  Example
//
//  Created by James Little on 11/4/21.
//  Copyright © 2021 Stripe. All rights reserved.
//

import Foundation
import StripeTerminal
import UIKit

class OfflineUIHandler {

    static var shared = OfflineUIHandler()
    var successfulForwardCount = 0
    var failedForwardCount = 0
    public var additionalDelegates = [OfflineDelegate]()

    // See SettingsViewController for where this class is set as the Terminal.offlineDelegate

    let rightBarButtonItemView: UIView = {
        let dimension: CGFloat = 10
        let view = UIView(frame: .init(origin: .zero, size: .init(width: dimension, height: dimension)))
        view.layer.cornerRadius = dimension / 2
        view.backgroundColor = .gray
        return view
    }()
}

extension OfflineUIHandler: OfflineDelegate {
    func terminal(_ terminal: Terminal, didChange offlineStatus: OfflineStatus) {
        let networkStatus = offlineStatus.sdk.networkStatus
        switch networkStatus {
        case .unknown:
            rightBarButtonItemView.backgroundColor = .gray
        case .offline:
            rightBarButtonItemView.backgroundColor = .red.withAlphaComponent(0.4)
        case .online:
            rightBarButtonItemView.backgroundColor = .green.withAlphaComponent(0.4)
        @unknown default:
            fatalError()
        }
        for delegate in additionalDelegates {
            delegate.terminal(terminal, didChange: offlineStatus)
        }
    }

    func terminal(_ terminal: Terminal, didForwardPaymentIntent intent: PaymentIntent, error: Error?) {
        if let error = error {
            failedForwardCount += 1

            // Show the error right away (these may stack)
            if let rootViewController = ((UIApplication.shared.delegate as? AppDelegate)?.window?.rootViewController as? RootViewController) {
                let labelOverlayView = LabelOverlayView(
                    labelText: "⚠️ Error forwarding payment \(intent.offlineDetails()?.stripeId ?? intent.stripeId ?? "N/A")\n\(error.localizedDescription)"
                )
                rootViewController.toastView(viewToToast: labelOverlayView)
            }
        } else {
            successfulForwardCount += 1
        }

        for delegate in additionalDelegates {
            delegate.terminal(terminal, didForwardPaymentIntent: intent, error: error)
        }

        // If we're done, report
        if Terminal.shared.offlineStatus.sdk.offlinePaymentsCount == 0 {
            reportForwardCountsAndReset()
        }
    }

    func terminal(_ terminal: Terminal, didReportForwardingError error: Error) {
        guard let rootViewController = ((UIApplication.shared.delegate as? AppDelegate)?.window?.rootViewController as? RootViewController) else { return }
        let labelOverlayView = LabelOverlayView(
            labelText: "⚠️ Error forwarding: \(error.localizedDescription)"
        )
        rootViewController.toastView(viewToToast: labelOverlayView)

        for delegate in additionalDelegates {
            delegate.terminal(terminal, didReportForwardingError: error)
        }
    }

    func reportForwardCountsAndReset() {
        guard let rootViewController = ((UIApplication.shared.delegate as? AppDelegate)?.window?.rootViewController as? RootViewController),
              (failedForwardCount > 0 || successfulForwardCount > 0) else {
            return
        }

        let text =
            """
            ✅ Forwarded \(successfulForwardCount) payment\(successfulForwardCount != 1 ? "s" : "")
            ⚠️ Failed to forward \(failedForwardCount) payment\(failedForwardCount != 1 ? "s" : "")
            """

        let labelOverlayView = LabelOverlayView(labelText: text)
        rootViewController.toastView(viewToToast: labelOverlayView)

        successfulForwardCount = 0
        failedForwardCount = 0
    }
}
