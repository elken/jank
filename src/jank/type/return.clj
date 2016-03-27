(ns jank.type.return
  (:require [jank.type.expression :as expression]
            [jank.type.declaration :as declaration])
  (:use clojure.pprint
        clojure.tools.trace
        jank.assert))

(defmulti add-explicit-returns
  "Adds explicit returns to if statements, lambdas, etc.
   Returns the modified item."
  (fn [item scope]
    (:kind item)))

(defmulti add-parameter-returns
  "Forces explicit returns for expressions being used as parameters.
   Not all items need to be modified when they're treated as parameters.
   The primary example of this is if expressions, which can be incomplete
   on their own, but must be complete when used as a parameter."
  (fn [item scope]
    (:kind item)))

(defmethod add-explicit-returns :lambda-definition
  [item scope]
  ; Don't bother redoing the work if we've already done it.
  ; The real reason we care is that this should only be done once per lambda,
  ; since the lambda only has access to its full scope once. If another item
  ; tries to do this again, with a lambda return, the appropriate scope will
  ; no longer be available.
  (if (= :return (-> item :body last :kind))
    item
    (let [expected-type (-> item :return :values first)]
      ; No return type means no implicit returns are generated. Nice.
      (if (nil? expected-type)
        item
        (let [updated-body (add-explicit-returns {:kind :body
                                                  :values (:body item)}
                                                 scope)
              body-type (expression/realize-type (last (:values updated-body))
                                                 scope)
              ; Allow deduction
              deduced-type (if (declaration/auto? expected-type)
                             body-type
                             expected-type)
              updated-item (assoc item :body (:values updated-body))]
          (type-assert (= deduced-type body-type)
                       (str "expected function return type of "
                            deduced-type
                            ", found "
                            body-type))

          ; Update the return type
          (assoc-in updated-item [:return :values] [deduced-type]))))))

(defmethod add-explicit-returns :if-expression
  [item scope]
  (type-assert (contains? item :else) "no else statement")

  (let [then-body (:values (add-explicit-returns
                             {:kind :body
                              :values [(:value (:then item))]}
                             scope))
        else-body (:values (add-explicit-returns
                             {:kind :body
                              :values [(:value (:else item))]}
                             scope))]
    (internal-assert (not-empty then-body)
                     "no return value in if/then expression")
    (internal-assert (not-empty else-body)
                     "no return value in if/else expression")

    (let [then-type (expression/realize-type (last then-body) scope)
          else-type (expression/realize-type (last else-body) scope)]
      (type-assert (= then-type else-type)
                   (str "incompatible if then/else types "
                        then-type
                        " and "
                        else-type))

      (assoc (assoc-in (assoc-in item [:then :values] then-body)
                       [:else :values] else-body)
             :type then-type))))

(defmethod add-explicit-returns :body
  [item scope]
  (let [body (:values item)]
    (let [updated-last (add-explicit-returns (last body) scope)]
      (assoc item :values (concat (butlast body)
                                  [{:kind :return
                                    :value updated-last}])))))

(defmethod add-explicit-returns :default
  [item scope]
  item)

(defmethod add-parameter-returns :if-expression
  [item scope]
  (add-explicit-returns item scope))

(defmethod add-parameter-returns :default
  [item scope]
  item)
